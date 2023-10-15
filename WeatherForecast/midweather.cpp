#include "midweather.h"
#include "ui_midweather.h"

MidWeather::MidWeather(QWidget *parent) : QWidget(parent), ui(new Ui::MidWeather)
{
    ui->setupUi(this);
    iniMember();//初始化成员变量

    //添加右键菜单
    mRightKeyMenu = new QMenu(this);
    mActExit = new QAction("刷新");
    mActExit->setIcon(QIcon(":/images/res/刷新.png"));
    mRightKeyMenu->addAction(mActExit);
    connect(mActExit,SIGNAL(triggered(bool)),this,SLOT(onMidRefresh()));

    //网络访问管理和天气工具类实例化
    mCityInfoTool = new CityInfoTool(this);
    mConfigInfoTool = new ConfigInfoTool(this);
    mNetworkAccessManager = new QNetworkAccessManager(this);
    connect(mNetworkAccessManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(onReply(QNetworkReply*)));
    connect(ui->memoLineEditInputCity, SIGNAL(returnPressed()), this, SLOT(on_memoBtnSearch_clicked()));

    getCityWeather(mConfigInfoTool->getConfigInfo("City"));
}

MidWeather::~MidWeather()
{
    emit midWeatherClose();
    delete ui;
}

//初始化成员变量
void MidWeather::iniMember()
{
    mCurrWeatherDateIndex = 0;//当前天气索引，默认是当天

    mCityInfoTool = Q_NULLPTR;//天气工具类
    mConfigInfoTool = Q_NULLPTR;//配置信息管理类
    mNetworkAccessManager = Q_NULLPTR;//网络访问管理

    mTimer = Q_NULLPTR;//实现显示定时器

    mBtnCanOp = true;//默认按钮可操作，避免数据拿取越界

    mRightKeyMenu = Q_NULLPTR;// 右键退出的菜单
    mActExit = Q_NULLPTR;// 键菜单-退出
}

/***********************************************************************
 * @brief: 根据据城市拿到天气信息
 * @param: QString cityName
 * @note:
 ***********************************************************************/
void MidWeather::getCityWeather(const QString & cityName)
{
    QString cityCode = mCityInfoTool->getCityCode(cityName);
    if(cityCode.isEmpty()){
        QMessageBox::information(this,"查询提示", "城市【" + cityName + "】不存在哦！",QMessageBox::Ok);
        return;
    }
    QUrl url(mConfigInfoTool->getRequestUrl() + cityCode);
    mNetworkAccessManager->get(QNetworkRequest(url));
}

/***********************************************************************
 * @brief: 响应网络请求结果
 * @param: QNetworkReply *reply
 * @note: 实时更新临时资源文件为最新获取数据，
 *        如果后期出现获取失败直接使用临时资源文件数据
 ***********************************************************************/
void MidWeather::onReply(QNetworkReply *reply)
{
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QFile file(mConfigInfoTool->getTempWeatherPath());
    QByteArray byteArray;

    //网络请求错误情况，读取临时资源文件中的内容
    if(reply->error() != QNetworkReply::NoError || 200 != statusCode){
        if(!file.open(QIODevice::ReadOnly)){
            if(file.open(QIODevice::ReadOnly)) //防止读取失败，再次读取
                byteArray = file.readAll();
        }else
            byteArray = file.readAll();
        file.close();

        bool isOK = parseUpYunJson(byteArray);//解析获取到的数据
        if(!isOK) //网络失败情况下，成功读取本地临时天气资源
            mBtnCanOp = false;
        else{
            upDateUI();//UI
            mBtnCanOp = true;
        }
        int autoUpdate = mConfigInfoTool->getConfigInfo("AutoUpdate").toInt();//自动更新
        if(0 == autoUpdate) //开启自动更新天气才去更新
            return;
        static int flag = 0;
        if(0 == flag)
            QMessageBox::warning(this,"天气","无网络连接，请检查网络！",QMessageBox::Ok);
        ++flag;
        if(12 == flag ) //一个小时提醒一次
            flag = 0;
    }else{  //网络请求正常情况
        byteArray = reply->readAll();
        //写入资源文件
        if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
            if(file.open(QIODevice::WriteOnly | QIODevice::Truncate))//防止写入失败，再次写入
                file.write(byteArray);
        }else
            file.write(byteArray);
        file.close();

        bool isOK = parseUpYunJson(byteArray);//解析获取到的数据
        if(!isOK) {//数据解析失败情况
            mBtnCanOp = false;
            QMessageBox::warning(this,"天气","数据更新失败，刷新一下哦~\n",QMessageBox::Ok);
        }
        else{
            upDateUI();//UI
            mBtnCanOp = true;
        }
    }
}

/***********************************************************************
 * @brief: 解析JSON文件；更新UI；温度绘图重绘事件
 * @param: QByteArray &byteArray
 * @note: 对应JSON格式解析
 ***********************************************************************/
bool MidWeather::parseUpYunJson(const QByteArray & byteArray)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(byteArray,&parseError);
    if(parseError.error != QJsonParseError::NoError)
        return false;

    QJsonObject rootObj = doc.object();
    QJsonObject rootData = rootObj.value("data").toObject();
    QJsonArray rootForeArray = rootData.value("forecast").toArray();
    for(int i=0;i<7;i++){
        QJsonObject rootForecast = rootForeArray[i].toObject();
        if(i==0){//当天特有的信息
            mWeatherData[i].city = rootObj.value("cityInfo").toObject().value("city").toString();
            mWeatherData[i].province = rootObj.value("cityInfo").toObject().value("parent").toString();
            mWeatherData[i].humidity = rootData.value("shidu").toString();
            mWeatherData[i].temperature = rootData.value("wendu").toString();
            mWeatherData[i].pM25 = rootData.value("pm25").toInt();
            mWeatherData[i].airQuality = rootData.value("quality").toString();
            mWeatherData[i].coldReminder = rootData.value("ganmao").toString();
        }else{
            mWeatherData[i].city = rootObj.value("cityInfo").toObject().value("city").toString();
            mWeatherData[i].province = rootObj.value("cityInfo").toObject().value("parent").toString();
            mWeatherData[i].humidity = "暂无";
            mWeatherData[i].temperature = "无";
            mWeatherData[i].pM25 = 0;
            mWeatherData[i].airQuality = "暂无";
            mWeatherData[i].coldReminder = "暂无";
        }
        mWeatherData[i].sunRise = rootForecast.value("sunrise").toString();
        mWeatherData[i].sunSet = rootForecast.value("sunset").toString();

        QString temp = rootForecast.value("high").toString().split(" ").at(1);
        mWeatherData[i].highTemperature = temp.left(temp.length() - 1);
        temp = rootForecast.value("low").toString().split(" ").at(1);
        mWeatherData[i].lowTemperature = temp.left(temp.length() - 1);

        mWeatherData[i].date = rootForecast.value("ymd").toString();
        mWeatherData[i].whichWeek = "周" + rootForecast.value("week").toString().right(1);
        mWeatherData[i].windDirection = rootForecast.value("fx").toString();
        mWeatherData[i].windPower = rootForecast.value("fl").toString();
        mWeatherData[i].weatherType = rootForecast.value("type").toString();
        mWeatherData[i].notice = rootForecast.value("notice").toString();
    }
    return true;
}

//默认刷新情况，不是按钮所为
void MidWeather::upDateUI() { btnUpDateUI(""); }

/***********************************************************************
 * @brief: 更新UI
 * @param: QString upDateType 整对按钮响应的更新，不是按钮传空串即可
 ***********************************************************************/
void MidWeather::btnUpDateUI(const QString & upDateType)
{
    //左上角的日期，保持最新状态
    //当前时间，保存最新状态
    QDateTime dateTime = QDateTime::currentDateTime();//获取当前系统时间
    QString zodiac = mConfigInfoTool->getCurrYearZodiac();//拿到今天的生肖
    if("" == zodiac)
        zodiac = " ";
    QString currDateWeek = zodiac + " "
            + dateTime.toString("yyyy-MM-dd") + " " +dateTime.toString("dddd").right(1);
    ui->memoCurrDateWeek->setText(currDateWeek);
    ui->memoCurrTime->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));

    //按钮更新操作
    if("左按钮" == upDateType){
        if(0 == mCurrWeatherDateIndex)
            mCurrWeatherDateIndex = 6;
        else
            --mCurrWeatherDateIndex;
    }
    if("右按钮" == upDateType){
        if(6 == mCurrWeatherDateIndex)
            mCurrWeatherDateIndex = 0;
        else
            ++mCurrWeatherDateIndex;
    }

    //获取当前时间，确定白天晚上天气图片
    QMap<QString,QString> weatherImage;
    int currHour = dateTime.toString("hh").toInt();
    int currMinute = dateTime.toString("mm").toInt();
    QString sunRiseHour = mWeatherData[mCurrWeatherDateIndex].sunRise.split(":").at(0);
    QString sunRiseMinute = mWeatherData[mCurrWeatherDateIndex].sunRise.split(":").at(1);
    QString sunSetHour = mWeatherData[mCurrWeatherDateIndex].sunSet.split(":").at(0);
    QString sunSetMinute = mWeatherData[mCurrWeatherDateIndex].sunSet.split(":").at(1);

    //判断并选择图片类型
    if(currHour > sunSetHour.toInt() || currHour < sunRiseHour.toInt())
        weatherImage = mSelectDayOrNightImage.value("晚上");
    else if(currHour == sunSetHour.toInt()){
        if(currMinute < sunSetMinute.toInt())
            weatherImage = mSelectDayOrNightImage.value("白天");
        else
            weatherImage = mSelectDayOrNightImage.value("晚上");
    }else if(currHour < sunSetHour.toInt() && currHour < sunRiseHour.toInt())
        weatherImage = mSelectDayOrNightImage.value("晚上");
    else if(currHour ==  sunRiseHour.toInt()){
        if(currMinute < sunRiseMinute.toInt())
            weatherImage = mSelectDayOrNightImage.value("白天");
        else
            weatherImage = mSelectDayOrNightImage.value("晚上");
    }else if(currHour > sunRiseHour.toInt() && currHour < sunSetHour.toInt())
        weatherImage = mSelectDayOrNightImage.value("白天");

    //中部
    ui->memoWeatherImage->setPixmap(weatherImage.value(mWeatherData[mCurrWeatherDateIndex].weatherType));
    ui->memoWeatherTemp->setText(mWeatherData[mCurrWeatherDateIndex].temperature + "°");

    //城市名称最多显示5个字符
    QString city = mWeatherData[0].city;
    if(city.length() <= 5)
        ui->memoBtnWeatherCity->setText(city);
    else
        ui->memoBtnWeatherCity->setText(city.left(4) + "...");

    if(0 == mCurrWeatherDateIndex)
        ui->memoLineEditInputCity->setPlaceholderText("今天的~");
    else if(1 == mCurrWeatherDateIndex)
        ui->memoLineEditInputCity->setPlaceholderText("明天的~");
    else{
        QString placeholderText = mWeatherData[mCurrWeatherDateIndex].whichWeek.right(1)
                + "/" + mWeatherData[mCurrWeatherDateIndex].date.right(2);
        ui->memoLineEditInputCity->setPlaceholderText(placeholderText + "的~");
    }

    //天气类型最多7个字符
    QString type = mWeatherData[mCurrWeatherDateIndex].weatherType;
    if(type.length() <= 7)
        ui->memoWeatherType->setText(type);
    else
        ui->memoWeatherType->setText(type.left(7) + "...");

    ui->memoTempRange->setText(mWeatherData[mCurrWeatherDateIndex].lowTemperature+ "°~"
                               + mWeatherData[mCurrWeatherDateIndex].highTemperature + "°");
    ui->memoSunRiseTime->setText("日出: " + mWeatherData[mCurrWeatherDateIndex].sunRise);
    ui->memoSunSetTime->setText("日落: " + mWeatherData[mCurrWeatherDateIndex].sunSet);

    //notice 一行只显示8个字符
    QString notice = mWeatherData[mCurrWeatherDateIndex].notice;
    int len = notice.length();
    if(len <= 8)
        ui->memoNotice->setText(notice);
    else{
        ui->memoNotice->setText(notice.left(8));
        ui->memoNotice_2->setText(notice.right( len % 8) );
    }
}

void MidWeather::contextMenuEvent(QContextMenuEvent *event)
{
    mRightKeyMenu->exec(QCursor::pos());//弹出右键菜单
    event->accept();
}

void MidWeather::on_memoBtnWeatherCity_clicked()
{
    //查询城市
    if(QDesktopServices::openUrl(QUrl(mConfigInfoTool->getBaiDuUrl() + ui->memoBtnWeatherCity->text())))
        return;
    if(QDesktopServices::openUrl(QUrl(mConfigInfoTool->getBingUrl() + ui->memoBtnWeatherCity->text())))
        return;
    if(QDesktopServices::openUrl(QUrl(mConfigInfoTool->get360Url() + ui->memoBtnWeatherCity->text())))
        return;
}

void MidWeather::on_memoBtnLeftSelect_clicked()
{
    if(mBtnCanOp)
        btnUpDateUI("左按钮");
}

void MidWeather::on_memoBtnRightSelect_clicked()
{
    if(mBtnCanOp)
        btnUpDateUI("右按钮");
}

void MidWeather::on_memoBtnClose_clicked() { this->close(); }

void MidWeather::on_memoBtnSearch_clicked()
{
    if(ui->memoLineEditInputCity->text().isEmpty())
        return;
    int autoUpdate = mConfigInfoTool->getConfigInfo("AutoUpdate").toInt();
    if(0 == autoUpdate) //开启自动更新天气才去更新
        return;
    getCityWeather(ui->memoLineEditInputCity->text());//查询城市
    ui->memoLineEditInputCity->clear();
}

//更新时间
void MidWeather::onMidTimeUpdate() { ui->memoCurrTime->setText(QDateTime::currentDateTime().toString("hh:mm:ss")); }\

//更新天气
void MidWeather::onMidRefresh()
{
    int autoUpdate = mConfigInfoTool->getConfigInfo("AutoUpdate").toInt();
    if(0 == autoUpdate) //开启自动更新天气才去更新
        return;
    getCityWeather(ui->memoBtnWeatherCity->text());
}

//初始化数据
void MidWeather::onWeatherForecastData(QMap<QString, QMap<QString, QString> > &selectDayOrNightImage,
                                       int currWeatherDateIndex)
{
    //初始化所需数据
    this->mSelectDayOrNightImage = selectDayOrNightImage;
    this->mCurrWeatherDateIndex = currWeatherDateIndex;
}

//窗口重绘，实现圆角什么什么的...
void MidWeather::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing,true);
    painter.setBrush(QColor(235, 232, 229));// Qt::white
    painter.setPen(Qt::transparent);
    QRect rect = this->rect();
    rect.setWidth(rect.width() - 1);
    rect.setHeight(rect.height() - 1);
    painter.drawRoundedRect(rect, 10, 10); //rect:绘制区域
    QWidget::paintEvent(event);
}

//窗口拖动：鼠标移动事件
void MidWeather::mouseMoveEvent(QMouseEvent *event)
{
    if(mZOffset == QPoint()) //防止按钮拖动导致的奇怪问题
        return;

    int isDrag = mConfigInfoTool->getConfigInfo("Min-IsDrag").toInt();
    if(1 == isDrag) //允许拖动
        this->move(event->globalPos() - mZOffset);
    QWidget::mouseMoveEvent(event);
}

//窗口拖动：鼠标按压事件
void MidWeather::mousePressEvent(QMouseEvent *event)
{
    this->setCursor(Qt::ClosedHandCursor);
    this->mZOffset = event->globalPos() - this->pos();
    QWidget::mousePressEvent(event);
}

//窗口拖动：鼠标释放事件
void MidWeather::mouseReleaseEvent(QMouseEvent *event)
{
    this->setCursor(Qt::ArrowCursor);
    mZOffset = QPoint();
    QWidget::mouseReleaseEvent(event);
}


