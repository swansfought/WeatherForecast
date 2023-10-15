#include "weatherforecast.h"
#include "ui_weatherforecast.h"

WeatherForecast::WeatherForecast(QWidget *parent) : QMainWindow(parent), ui(new Ui::WeatherForecast)
{
    ui->setupUi(this);
    iniMember();//初始化成员变量

    //设置窗口属性
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);//设置窗口背景透明
    setFixedSize(width(), height()); // 设置固定窗口大小
    setWindowTitle("SimpleWeather");

    //网络访问管理、天气工具类、配置信息工具类实例化
    mCityInfoTool = new CityInfoTool(this);
    mConfigInfoTool = new ConfigInfoTool(this);
    mNetworkAccessManager = new QNetworkAccessManager(this);
    connect(mNetworkAccessManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(onReply(QNetworkReply*)));//响应网络请求结果

    addDateWeekLabelToList(); //添加部分Label控件
    addMapWeatherImage();//初始化天气图片Map
    createystemTrayIcon();//添加系统托盘和右键菜单
    createRightMenu();//构建右键菜单

    connect(ui->lineEditInputCity, SIGNAL(returnPressed()), this, SLOT(on_btnSearch_clicked()));//回车搜索
    ui->tempWidget->installEventFilter(this); //添加widget事件

    //当前时间，保存最新状态
    mTimer = new QTimer(this);
    ui->labelCurrTime->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
    connect(mTimer,SIGNAL(timeout()),this,SLOT(onTimeUpdate()));
    mTimer->start(1000);//1s

    //-------------------------------------------------------
    //添加文件保存和天气更新定时器(共用一个)
    mTimerThread = new QThread;//已在析构函数中处理
    mSaveAndUpdateTimer = new QTimer;//已在析构函数中处理

    int autoUpdateTime = mConfigInfoTool->getConfigInfo("WeatherAutoUpdateTime_min").toInt();
    mSaveAndUpdateTimer->setInterval(1000 * 60 * autoUpdateTime);
    mSaveAndUpdateTimer->moveToThread(mTimerThread);

    connect(mSaveAndUpdateTimer,SIGNAL(timeout()),this,SLOT(onTimeOutSaveFile()));//保存文件
    connect(mSaveAndUpdateTimer,SIGNAL(timeout()),this,SLOT(onTimeOutUpdateWeather())); //更新天气
    connect(mTimerThread, SIGNAL(started()), mSaveAndUpdateTimer,SLOT(start()));//开启定时器
    connect(mTimerThread,SIGNAL(finished()),mSaveAndUpdateTimer,SLOT(stop()));//停止定时器
    mTimerThread->start();//启动线程
    //-------------------------------------------------------

    QString winModel = mConfigInfoTool->getConfigInfo("WinModel");//窗口模式
    if("标准模式" == winModel){
        getCityWeather(mConfigInfoTool->getConfigInfo("City"));
        on_btnRightRead_clicked();//获取文本信息
    }else if("天气小窗口" == winModel) //搭配主窗口不显示情况
        onCreateMidWindow();
    else if("备忘录窗口" == winModel)
        onCreateMemoWindow();
}

WeatherForecast::~WeatherForecast()
{
    on_btnLeftSave_clicked();//退出之前确保文件保存

    mTimerThread->quit();//退出进程
    mTimerThread->wait();//等待定时器停止

    if(Q_NULLPTR != mMidWindow)  //析构天气小窗口和Memo窗口
        mMidWindow->deleteLater();
    if(Q_NULLPTR != mMemoWindow)
        mMemoWindow->deleteLater();

    if(Q_NULLPTR != mSaveAndUpdateTimer) //析构定时器和线程
        mSaveAndUpdateTimer->deleteLater();
    if(Q_NULLPTR != mTimerThread)
        mTimerThread->deleteLater();

    delete ui;
}

//初始化成员变量
void WeatherForecast::iniMember()
{
    mCurrWeatherDateIndex = 0;//当前天气索引，默认是当天

    mTimer = Q_NULLPTR; //时间显示定时器，主窗口与天气小窗口共用，1s
    mSaveAndUpdateTimer = Q_NULLPTR; //文件保存,天气更新定时器，主窗口与memo小窗口共用
    mTimerThread = Q_NULLPTR;

    mCityInfoTool = Q_NULLPTR;//天气工具类
    mConfigInfoTool = Q_NULLPTR;//配置信息工具类
    mNetworkAccessManager = Q_NULLPTR;//网络访问管理

    mSystemTrayIcon = Q_NULLPTR;//系统托盘
    mSystemTrayMenu = Q_NULLPTR;// 右键退出的菜单

    mRightKeyMenu = Q_NULLPTR;// 右键退菜单
    mActRefresh = Q_NULLPTR;// 键菜单-刷新
    mActMidWindow = Q_NULLPTR;//右键菜单-小天气窗口
    mActMemoWindow = Q_NULLPTR;//右键菜单-memo窗口

    mWinIsTop = false;//小天气窗口mMemo窗口置顶置底标识符，默认置底状态
    mBtnCanOp = true;//默认按钮可操作，避免数据拿取越界

    mConfigInfo = Q_NULLPTR;//设置窗口
    mMidWindow = Q_NULLPTR;//中间小窗口
    mMemoWindow = Q_NULLPTR;//备忘录窗口
}

/***********************************************************************
 * @brief: 更根据城市拿到天气信息
 * @param: QString cityName
 * @note: 完成跳转至槽函数 onReply(QNetworkReply *reply)
 ***********************************************************************/
void WeatherForecast::getCityWeather(const QString & cityName)
{
    QString cityCode = mCityInfoTool->getCityCode(cityName);
    if(cityCode.isEmpty()){
        QMessageBox::information(this,"查询提示", "城市【" + cityName + "】不存在哦！",QMessageBox::Ok);
        return;
    }
    QUrl url(mConfigInfoTool->getRequestUrl() + cityCode);
    mNetworkAccessManager->get(QNetworkRequest(url));//get请求
}

/***********************************************************************
 * @brief: 响应网络请求结果；更新UI；温度绘图重绘事件
 * @param: QNetworkReply *reply
 * @note: 更新临时资源文件为最新获取数据，如果获取失败直接使用临时资源文件数据
 ***********************************************************************/
void WeatherForecast::onReply(QNetworkReply *reply)
{
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QFile file(mConfigInfoTool->getTempWeatherPath());
    QByteArray byteArray;

    //网络请求错误情况，读取临时资源文件中的内容
    if(reply->error() != QNetworkReply::NoError || 200 != statusCode){
        if(!file.open(QIODevice::ReadOnly)){
            if(file.open(QIODevice::ReadOnly))//防止读取失败，再次读取
                byteArray = file.readAll();
        }else
            byteArray = file.readAll();
        file.close();

        bool isOK = parseUpYunJson(byteArray);//解析获取到的数据
        if(!isOK)
            mBtnCanOp = false;
        else{ //成功读取本地临时天气资源
            mBtnCanOp = true; //按钮可以操作
            upDateUI(); //UI
            ui->tempWidget->update();//产生温度曲线图更新时间
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
    }else{ //网络请求正常情况
        byteArray = reply->readAll();
        //写入资源文件
        if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
            if(file.open(QIODevice::WriteOnly | QIODevice::Truncate))//防止写入失败，再次写入
                file.write(byteArray);
        }else
            file.write(byteArray);
        file.close();

        bool isOK = parseUpYunJson(byteArray);//解析获取到的数据
        if(!isOK){ //数据解析失败情况
            mBtnCanOp = false;
            QMessageBox::information(this,"天气","数据更新失败，刷新一下哦~\n",QMessageBox::Ok);
        }else{
            upDateUI();//UI
            ui->tempWidget->update();//产生温度曲线图更新时间
            mBtnCanOp = true;
        }
    }
}

/***********************************************************************
 * @brief: 解析UpYun JSON文件
 * @param: QByteArray &byteArray
 ***********************************************************************/
bool WeatherForecast::parseUpYunJson(const QByteArray & byteArray)
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
            mWeatherData[i].city = rootObj.value("cityInefo").toObject().value("city").toString();
            mWeatherData[i].province = rootObj.value("cityInfo").toObject().value("parent").toString();
            mWeatherData[i].humidity = "无~";
            mWeatherData[i].temperature = "无";
            mWeatherData[i].pM25 = 0;
            mWeatherData[i].airQuality = "无~";
            mWeatherData[i].coldReminder = "无~";
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

//默认更新UI，不是按钮所为
void WeatherForecast::upDateUI() { btnUpDateUI(""); }

//更新UI
void WeatherForecast::btnUpDateUI(const QString & upDateType)
{
    //左上角的日期，保持最新状态
    QDateTime dateTime = QDateTime::currentDateTime();//获取当前系统时间
    QString zodiac = mConfigInfoTool->getCurrYearZodiac();//拿到今天的生肖
    if("" == zodiac)
        zodiac = " ";
    QString currDateWeek = zodiac + " " + dateTime.toString("yyyy-MM-dd");
    ui->labelCurrDateWeek->setText(currDateWeek);

    //按钮更新操作
    if("左按钮" == upDateType){
        if(0 == mCurrWeatherDateIndex)
            mCurrWeatherDateIndex = 6;
        else
            --mCurrWeatherDateIndex;
    }else if("右按钮" == upDateType){
        if(6 == mCurrWeatherDateIndex)
            mCurrWeatherDateIndex = 0;
        else
            ++mCurrWeatherDateIndex;
    }

    //左部 - 手动格式化感冒指数输出。最多七行（\n），不过一般最后三四行。n
    QString clodReminder = mWeatherData[mCurrWeatherDateIndex].coldReminder;
    int size = clodReminder.length(); //萨达萨达阿的松松\n\n\n\n\n\n
    if(size <= 8)
        ui->labelClodReminder->setText(clodReminder + "\n\n\n\n\n\n");
    else{
        QString temp = "";
        for(int i=0;i<6;i++){
            if(i<(size/8))
                temp += clodReminder.mid(i*8,8) + "\n";
            else if(i>=(size/8) && (size%8) > 0)
                temp += clodReminder.mid(i*8, size%8) + "\n";//补上剩下的
            else
                temp += "\n";
        }
        ui->labelClodReminder->setText(temp);
    }

    //获取当前时间，确定白天晚上天气图片
    QMap<QString,QString> weatherImage;
    int currHour =dateTime.toString("hh").toInt();
    int currMinute =dateTime.toString("mm").toInt();
    QString sunRiseHour = mWeatherData[mCurrWeatherDateIndex].sunRise.split(":").at(0);
    QString sunRiseMinute = mWeatherData[mCurrWeatherDateIndex].sunRise.split(":").at(1);
    QString sunSetHour = mWeatherData[mCurrWeatherDateIndex].sunSet.split(":").at(0);
    QString sunSetMinute = mWeatherData[mCurrWeatherDateIndex].sunSet.split(":").at(1);

    //判断并选择天气图片
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
    ui->labelWeatherImage->setPixmap(weatherImage.value(mWeatherData[mCurrWeatherDateIndex].weatherType));
    ui->labelCurrTemp->setText(mWeatherData[mCurrWeatherDateIndex].temperature + "°");

    //城市名称最多显示5个字符
    QString city = mWeatherData[0].city;
    if(city.length() <= 5)
        ui->btnWeatherCity->setText(city);
    else
        ui->btnWeatherCity->setText(city.left(4) + "...");

    //天气类型最多7个字符
    QString type = mWeatherData[mCurrWeatherDateIndex].weatherType;
    if(type.length() <= 7)
        ui->labelWeatherType->setText(type);
    else
        ui->labelWeatherType->setText(type.left(7) + "...");

    ui->labelTempRange->setText(mWeatherData[mCurrWeatherDateIndex].lowTemperature+ "°~"
                                + mWeatherData[mCurrWeatherDateIndex].highTemperature + "°");
    ui->labelSunRiseTime->setText("日出: " + mWeatherData[mCurrWeatherDateIndex].sunRise);
    ui->labelSunSetTime->setText("日落: " + mWeatherData[mCurrWeatherDateIndex].sunSet);

    //nitice 一行只显示8个字符
    QString notice = mWeatherData[mCurrWeatherDateIndex].notice;
    int len = notice.length();
    if(len <= 8)
        ui->labelNotice->setText(notice);
    else{
        ui->labelNotice->setText(notice.left(8));
        ui->labelNotice_2->setText(notice.right( len % 8) );
    }

    //右部 - 风向风力、湿度、PM2.5和空气质量
    ui->labelWindDeractionTitle->setText(mWeatherData[mCurrWeatherDateIndex].windDirection);
    ui->labelWindDeractionValue->setText(mWeatherData[mCurrWeatherDateIndex].windPower);
    ui->labelHumidityValue->setText(mWeatherData[mCurrWeatherDateIndex].humidity);
    if(0 == mCurrWeatherDateIndex)
        ui->labelPM25Value->setText(QString::number(mWeatherData[0].pM25));
    else
        ui->labelPM25Value->setText("无~");//覆盖参数的 0
    ui->labelAirQualityValue->setText(mWeatherData[mCurrWeatherDateIndex].airQuality);

    //设置风力图片等级颜色背景
    int windPower = mWeatherData[mCurrWeatherDateIndex].windPower.left(1).toInt();
    if(windPower > 0 && windPower <= 2)
        ui->labelWindDeractionImage->setStyleSheet("background-color:rgb(150,222,209)");
    else if(windPower > 2 && windPower <= 5)
        ui->labelWindDeractionImage->setStyleSheet("background-color:rgb(64,181,173)");
    else if(windPower > 5 && windPower <= 7)//蓝色预警
        ui->labelWindDeractionImage->setStyleSheet("background-color:rgb(52,101,255)");
    else if(windPower > 7 && windPower <= 9)//黄色预警
        ui->labelWindDeractionImage->setStyleSheet("background-color:rgb(240,255,12)");
    else if(windPower > 9 && windPower <= 11)//橙色预警
        ui->labelWindDeractionImage->setStyleSheet("background-color:rgb(254,153,0)");
    else if(windPower > 11)//红色预警
        ui->labelWindDeractionImage->setStyleSheet("background-color:rgb(216,47,44)");

    //下面图标颜色就设置当天的，因为后面无数据
    if(0 == mCurrWeatherDateIndex){
        //湿度等级设置
        QString   strHumidity= mWeatherData[mCurrWeatherDateIndex].humidity;
        int humidity = strHumidity.left(strHumidity.length() - 1).toInt();
        if(humidity > 0 && humidity <= 25)
            ui->labelHumidityImage->setStyleSheet("background-color:rgb(197,255,231)");
        else if(humidity > 25 && humidity <= 50)
            ui->labelHumidityImage->setStyleSheet("background-color:rgb(99,255,254)");
        else if(humidity > 50 && humidity <= 75)
            ui->labelHumidityImage->setStyleSheet("background-color:rgb(1,207,205)");
        else if(humidity > 75 && humidity <= 100)
            ui->labelHumidityImage->setStyleSheet("background-color:rgb(0,154,156)");
        else if(humidity > 100)
            ui->labelHumidityImage->setStyleSheet("background-color:rgb(1,83,77)");

        //空气质量等级设置
        QString airQuality = mWeatherData[0].airQuality;
        if("优" == airQuality){
            ui->labelPM25Image->setStyleSheet("background-color:rgb(64,192,87)");
            ui->labelAirQualityImage->setStyleSheet("background-color:rgb(64,192,87)");
        }
        else if("良" == airQuality){
            ui->labelPM25Image->setStyleSheet("background-color:rgb(130,201,30)");
            ui->labelAirQualityImage->setStyleSheet("background-color:rgb(130,201,30)");
        }
        else if("轻度" == airQuality){
            ui->labelPM25Image->setStyleSheet("background-color:rgb(247,103,7)");
            ui->labelAirQualityImage->setStyleSheet("background-color:rgb(247,103,7)");
        }
        else if("中度" == airQuality){
            ui->labelPM25Image->setStyleSheet("background-color:rgb(224,49,49)");
            ui->labelAirQualityImage->setStyleSheet("background-color:rgb(224,49,49)");
        }
        else if("重度" == airQuality){
            ui->labelPM25Image->setStyleSheet("background-color:rgb(148,112,220)");
            ui->labelAirQualityImage->setStyleSheet("background-color:rgb(148,112,220)");
        }
        else if("严重" == airQuality){
            ui->labelPM25Image->setStyleSheet("background-color:rgb(132,2,0)");
            ui->labelAirQualityImage->setStyleSheet("background-color:rgb(132,2,0)");
        }
    }

    //底部-星期和日期时间设置
    QString week;
    for(int i=0;i<7;i++){
        if(0 == i)
            mBottomWeek[0]->setText("今天");
        else if(1 == i )
            mBottomWeek[1]->setText("明天");
        else{
            week = mWeatherData[i].whichWeek;
            if("周一" == week)
                mBottomWeek[i]->setText("Mon");
            else if("周二" == week)
                mBottomWeek[i]->setText("Tue");
            else if("周三" == week)
                mBottomWeek[i]->setText("Wed");
            else if("周四" == week)
                mBottomWeek[i]->setText("Thu");
            else if("周五" == week)
                mBottomWeek[i]->setText("Fri");
            else if("周六" == week)
                mBottomWeek[i]->setText("Sat");
            else if("周日" == week)
                mBottomWeek[i]->setText("Sun");
        }
        mBottomDate[i]->setText(mWeatherData[i].date.right(5));

        //周和日期样式
        if(mCurrWeatherDateIndex == i){
            mBottomWeek[i]->setStyleSheet("QLabel{color: rgb(44,44,44);border-top-left-radius:5px;"
                                          "border-top-right-radius:5px;border-bottom-left-radius:0px;"
                                          "border-bottom-right-radius:0px;background-color:rgb(234, 237, 238);"
                                          "border-style: solid;border-width: 2px;border-color: rgb(44,44,44);}");
            mBottomDate[i]->setStyleSheet("QLabel{color: rgb(44,44,44);border-top-left-radius:0px;"
                                          "border-top-right-radius:0px;border-bottom-left-radius:5px;"
                                          "border-bottom-right-radius:5px;background-color:rgb(234, 237, 238);"
                                          "border-style: groove;border-width: 2px;border-color: rgb(44,44,44);}");
        }else{
            mBottomWeek[i]->setStyleSheet("QLabel{border-radius:5px;color: rgb(44, 44, 44);}");
            mBottomDate[i]->setStyleSheet("QLabel{border-radius:5px;color: rgb(44, 44, 44);}");
        }
    }
}

/***********************************************************************
 * @brief: 添加事件过滤器，目的是绘制温度折线图
 * @param: QObject *obj, QEvent *even
 * @note: 判断类型不要写错 [event->type() == QEvent::Paint]
 ***********************************************************************/
bool WeatherForecast::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == ui->tempWidget && event->type() == QEvent::Paint)
        painTemperatureCurve();
    return QWidget::eventFilter(watched,event);
}

/***********************************************************************
 * @brief: 绘制温度折线图
 * @note: 如果高低温差距过大可能会导致温度值显示不全问题
 ***********************************************************************/
void WeatherForecast::painTemperatureCurve()
{
    QPainter painter(ui->tempWidget);
    painter.setRenderHint(QPainter::Antialiasing);//抗锯齿
    painter.setRenderHint(QPainter::TextAntialiasing);

    int xPoint[7] = {0}; //x轴坐标
    int i;
    int sumHighTemp = 0,sumLowTemp = 0;
    int yHighCenter = ui->tempWidget->height()/2 - TEMP_HIGH_Y_OFFSET; //设置高温y轴中间值
    int yLowCenter = ui->tempWidget->height()/2 + TEMP_LOW_Y_OFFSET;//设置低温y轴中间值
    for(i=0;i<7;i++){
        xPoint[i] = mBottomWeek[i]->pos().x() + mBottomWeek[i]->width()/2;//x轴坐标
        sumHighTemp += mWeatherData[i].highTemperature.toInt();//高温总和
        sumLowTemp += mWeatherData[i].lowTemperature.toInt();//低温总和
    }
    //计算距离平均值的y轴偏移量
    int avgHighTemp = sumHighTemp/7;//高温平均值
    int avgLowTemp = sumLowTemp/7;//低温平均值
    int yHighPoint[7] = {0}; //高温y轴坐标
    int yLowPoint[7] = {0}; //低温y轴坐标
    for(i=0;i<7;i++){
        yHighPoint[i] = yHighCenter - (mWeatherData[i].highTemperature.toInt() - avgHighTemp)*TEMP_INCREASE_PIXEL;
        yLowPoint[i] = yLowCenter - (mWeatherData[i].lowTemperature.toInt() - avgLowTemp)*TEMP_INCREASE_PIXEL;
    }

    //画高温点和线
    QPen pen;
    pen.setWidth(3);//2
    pen.setColor(QColor(238, 78, 83));
    painter.setPen(pen);
    QBrush brush(QColor(238, 78, 83));
    painter.setBrush(brush);//点
    for(i=0;i<7;i++)  //画点
        painter.drawEllipse(QPoint(xPoint[i], yHighPoint[i]), TEMP_POINT_SIZE, TEMP_POINT_SIZE);
    for(i=0;i<6;i++) //连线
        painter.drawLine(xPoint[i], yHighPoint[i], xPoint[i+1], yHighPoint[i+1]);

    //画低温点和线
    pen.setColor(QColor(8,103,165));
    painter.setPen(pen);
    brush.setColor(QColor(8,103,165));
    painter.setBrush(brush);
    for(i=0;i<7;i++) //画点
        painter.drawEllipse(QPoint(xPoint[i],yLowPoint[i]),TEMP_POINT_SIZE,TEMP_POINT_SIZE);
    for(i=0;i<6;i++) //连线
        painter.drawLine(xPoint[i],yLowPoint[i],xPoint[i+1],yLowPoint[i+1]);

    //画高温和低温的温度值
    pen.setColor(QColor(44,44,44));
    painter.setPen(pen);
    for(i=0;i<7;i++){
        //高温值
        painter.drawText(xPoint[i] - TEMP_HIGH_TEXT_X_OFFSET,
                         yHighPoint[i] - TEMP_HIGH_TEXT_Y_OFFSET,mWeatherData[i].highTemperature + "°");
        //低温值
        painter.drawText(xPoint[i] - TEMP_LOW_TEXT_X_OFFSET,
                         yLowPoint[i] + TEMP_LOW_TEXT_Y_OFFSET, mWeatherData[i].lowTemperature + "°");
    }

    //当天的高地温度连线
    pen.setStyle(Qt::DotLine);
    int diff = (yHighPoint[mCurrWeatherDateIndex] - yLowPoint[mCurrWeatherDateIndex])/2;
    pen.setColor(QColor(238, 78, 83));
    painter.setPen(pen);
    painter.drawLine(xPoint[mCurrWeatherDateIndex],yHighPoint[mCurrWeatherDateIndex],
                     xPoint[mCurrWeatherDateIndex],yHighPoint[mCurrWeatherDateIndex] - diff);
    pen.setColor(QColor(8,103,165));
    painter.setPen(pen);
    painter.drawLine(xPoint[mCurrWeatherDateIndex],yHighPoint[mCurrWeatherDateIndex] - diff,
                     xPoint[mCurrWeatherDateIndex],yLowPoint[mCurrWeatherDateIndex]);
}

/***********************************************************************
 * @brief: 创建右键菜单
 * @note: 解决缩小窗口之后右击菜单透明问题，现在暂无(窗口嵌入到window桌面下的原因???)
 ***********************************************************************/
void WeatherForecast::createRightMenu()
{
    if(Q_NULLPTR == mRightKeyMenu) //先创建右键菜单
        mRightKeyMenu = new QMenu(this);

    //创建，绑定mActRefresh
    if(Q_NULLPTR == mActRefresh && Q_NULLPTR != mRightKeyMenu){
        mActRefresh = new QAction("刷新");
        mActRefresh->setIcon(QIcon(":/images/res/刷新.png"));
        mRightKeyMenu->addAction(mActRefresh);
        connect(mActRefresh,SIGNAL(triggered(bool)),this,SLOT(onRefresh()));
    }

    //小窗口模式
    int isCreateSmallWindow = mConfigInfoTool->getConfigInfo("SmallWindow").toInt();
    if(0 == isCreateSmallWindow) //创建，绑定mActMidWindow
        return;
    if(Q_NULLPTR == mActMidWindow){
        mActMidWindow = new QAction("小窗口");
        mActMidWindow->setIcon(QIcon(":/images/res/Application.ico"));
        mRightKeyMenu->addAction(mActMidWindow);
        connect(mActMidWindow,SIGNAL(triggered(bool)),this,SLOT(onCreateMidWindow()));
    }
    if(Q_NULLPTR == mActMemoWindow){ //创建，绑定mActMemoWindow
        mActMemoWindow = new QAction("备忘录");
        mActMemoWindow->setIcon(QIcon(":/images/res/memo.png"));
        mRightKeyMenu->addAction(mActMemoWindow);
        connect(mActMemoWindow,SIGNAL(triggered(bool)),this,SLOT(onCreateMemoWindow()));
    }
    //    mExitAct = new QAction();
    //    mExitAct->setText("退出");
    //    mExitAct->setIcon(QIcon(":/res/close.png"));
    //    mExitMenu->addAction(mExitAct);
    //    简写方式
    //    connect(mExitAct, QAction::triggered, this, [=] { qApp->exit(0); });
    //    connect(mRightKeyMenu,QAction::trigger,this,onRefresh);
}

/***********************************************************************
 * @brief: 关闭右键菜单
 * @note:  解决缩小窗口之后右击菜单透明问题，现在暂无(窗口嵌入到window桌面下的原因???)
 ***********************************************************************/
void WeatherForecast::closeRightMenu()
{
    if(Q_NULLPTR != mActRefresh){ //解绑mActRefresh，删除
        disconnect(mActRefresh,SIGNAL(triggered(bool)),this,SLOT(onRefresh()));
        mRightKeyMenu->removeAction(mActRefresh);
        mActRefresh = Q_NULLPTR;
    }
    if(Q_NULLPTR != mActMidWindow){ //解绑mActMidWindow，删除
        disconnect(mActMidWindow,SIGNAL(triggered(bool)),this,SLOT(onCreateMidWindow()));
        mRightKeyMenu->removeAction(mActMidWindow);
        mActMidWindow = Q_NULLPTR;
    }
    if(Q_NULLPTR != mActMemoWindow){ //解绑mActMemoWindow，删除
        disconnect(mActMemoWindow,SIGNAL(triggered(bool)),this,SLOT(onCreateMemoWindow()));
        mRightKeyMenu->removeAction(mActMemoWindow);
        mActMemoWindow = Q_NULLPTR;
    }
    if(Q_NULLPTR != mRightKeyMenu){ //最后删除右键菜单
        mRightKeyMenu->deleteLater();
        mRightKeyMenu = Q_NULLPTR;
    }
}

/***********************************************************************
 * @brief: 响应处理设置窗口是否开启小窗口模式
 * @param: int isCreateSmallWindow   0不开启；1开启
 * @note: 创建两个action加到右键菜单中去
 ***********************************************************************/
void WeatherForecast::onCreateMenus()
{
    //只创建一次
    int isCreateSmallWindow = mConfigInfoTool->getConfigInfo("SmallWindow").toInt();
    if(1 == isCreateSmallWindow && mActMidWindow == Q_NULLPTR && mActMemoWindow == Q_NULLPTR){
        mActMidWindow = new QAction("小窗口");
        mActMidWindow->setIcon(QIcon(":/images/res/Application.ico"));
        mActMemoWindow = new QAction("备忘录");
        mActMemoWindow->setIcon(QIcon(":/images/res/memo.png"));
        mRightKeyMenu->addAction(mActMidWindow);
        mRightKeyMenu->addAction(mActMemoWindow);
        connect(mActMidWindow,SIGNAL(triggered(bool)),this,SLOT(onCreateMidWindow()));
        connect(mActMemoWindow,SIGNAL(triggered(bool)),this,SLOT(onCreateMemoWindow()));
        return;
    }
    if(0 == isCreateSmallWindow){
        if(Q_NULLPTR == mActMidWindow && Q_NULLPTR == mActMemoWindow)//自始至终没开启
            return;
        disconnect(mActMidWindow,SIGNAL(triggered(bool)),this,SLOT(onCreateMidWindow()));
        disconnect(mActMemoWindow,SIGNAL(triggered(bool)),this,SLOT(onCreateMemoWindow()));
        mRightKeyMenu->removeAction(mActMidWindow);
        mRightKeyMenu->removeAction(mActMemoWindow);
        mActMidWindow = Q_NULLPTR;
        mActMemoWindow = Q_NULLPTR;
        return;
    }
}

//刷新，再次请求新数据
void WeatherForecast::onRefresh()
{
    //开启自动更新天气才去更新
    int autoUpdate = mConfigInfoTool->getConfigInfo("AutoUpdate").toInt();
    if(0 == autoUpdate)
        return;
    getCityWeather(ui->btnWeatherCity->text());
}

/***********************************************************************
 * @brief: 添加系统托盘和右键菜单
 * @note:  QCommonStyle style;系统标准图片
 ***********************************************************************/
void WeatherForecast::createystemTrayIcon()
{
    mSystemTrayIcon = new QSystemTrayIcon(this);
    mSystemTrayMenu = new QMenu(this);
    //    QCommonStyle style;
    //    mSystemTrayMenu->addAction(QIcon(style.standardPixmap(QStyle::SP_DockWidgetCloseButton)),"退出");
    mSystemTrayMenu->addAction(QIcon(":/images/res/退出.png"),"退出");
    mSystemTrayIcon->setContextMenu(mSystemTrayMenu);
    mSystemTrayIcon->setIcon(QIcon(":/images/res/Application.ico"));
    mSystemTrayIcon->setToolTip("SimpleWeather");
    mSystemTrayIcon->show();
    connect(mSystemTrayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this,SLOT(onActivated(QSystemTrayIcon::ActivationReason)));
    connect(mSystemTrayMenu,SIGNAL(triggered(QAction*)),this,SLOT(onTiggered(QAction*)));
}

//系统托盘右击菜单
void WeatherForecast::onTiggered(QAction *action)
{
    if("退出" != action->text())
        return;

    //如果是Memo窗口情况，通知memo窗口保存数据
    if(Q_NULLPTR != mMemoWindow){
        connect(this,SIGNAL(mainClose()),mMemoWindow,SLOT(onMainClose()));
        emit mainClose();
    }
    qApp->quit();
}

//系统托盘图标被点击
void WeatherForecast::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    //针对不同窗口点击事件处理
    if(reason == QSystemTrayIcon::Trigger){
        //重置主界面
        if(Q_NULLPTR == mMidWindow && Q_NULLPTR == mMemoWindow){
            //这里是针对配置文件开启了主窗口可拖动的功能
            int ax = mConfigInfoTool->getConfigInfo("Max-XPos").toInt();
            int ay = mConfigInfoTool->getConfigInfo("Max-YPos").toInt();
            this->move(ax, ay);
            this->show();//针对关闭窗口之后显示
            createRightMenu();//创建右键菜单
            return;
        }
        //重置小窗口
        if(Q_NULLPTR != mMidWindow && Q_NULLPTR == mMemoWindow){
            int ax = mConfigInfoTool->getConfigInfo("Min-XPos").toInt();
            int ay = mConfigInfoTool->getConfigInfo("Min-YPos").toInt();
            mMidWindow->move(ax,ay);
            return;
        }
        //重置备忘录
        if(Q_NULLPTR != mMemoWindow && Q_NULLPTR == mMidWindow){
            int ax = mConfigInfoTool->getConfigInfo("Memo-XPos").toInt();
            int ay = mConfigInfoTool->getConfigInfo("Memo-YPos").toInt();
            mMemoWindow->move(ax,ay);
            return;
        }
    }

    //Memo窗口双击事件处理
    if(reason == QSystemTrayIcon::DoubleClick){
        if(!mWinIsTop){ //置顶窗口
            mWinIsTop = true;
            if(Q_NULLPTR != mMemoWindow && Q_NULLPTR == mMidWindow){ //Memo窗口
                //先取消置底，再置顶
                mMemoWindow->setWindowFlags(mMemoWindow->windowFlags() & ~Qt::WindowStaysOnBottomHint);
                mMemoWindow->setWindowFlags(mMemoWindow->windowFlags() | Qt::WindowStaysOnTopHint);
                mMemoWindow->show();
                return;
            }
            if(Q_NULLPTR != mMidWindow && Q_NULLPTR == mMemoWindow){ //小天气窗口
                mMidWindow->setWindowFlags(mMidWindow->windowFlags() & ~Qt::WindowStaysOnBottomHint);
                mMidWindow->setWindowFlags(mMidWindow->windowFlags() | Qt::WindowStaysOnTopHint);
                mMidWindow->show();
                return;
            }
        }else{ //取消置顶
            mWinIsTop = false;
            if(Q_NULLPTR != mMemoWindow && Q_NULLPTR == mMidWindow){ //Memo窗口
                mMemoWindow->setWindowFlags(mMemoWindow->windowFlags() & ~Qt::WindowStaysOnTopHint);
                mMemoWindow->setWindowFlags(mMemoWindow->windowFlags() | Qt::WindowStaysOnBottomHint);
                mMemoWindow->show();
            }
            if(Q_NULLPTR != mMidWindow && Q_NULLPTR == mMemoWindow){ //小天气窗口
                mMidWindow->setWindowFlags(mMidWindow->windowFlags() & ~Qt::WindowStaysOnTopHint);
                mMidWindow->setWindowFlags(mMidWindow->windowFlags() | Qt::WindowStaysOnBottomHint);
                mMidWindow->show();
            }
        }
    }
}

//更新时间
void WeatherForecast::onTimeUpdate()
{
    ui->labelCurrTime->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
}

//将日期和星期的控件加入list
void WeatherForecast::addDateWeekLabelToList()
{
    //底部右边：星期控件
    mBottomWeek<<ui->labelWeek0<<ui->labelWeek1<<ui->labelWeek2<<ui->labelWeek3
              <<ui->labelWeek4<<ui->labelWeek5<<ui->labelWeek6;
    //底部右边：日期控件
    mBottomDate<<ui->labelData0<<ui->labelData1<<ui->labelData2<<ui->labelData3
              <<ui->labelData4<<ui->labelData5<<ui->labelData6;
}

//搜索按钮被点击
void WeatherForecast::on_btnSearch_clicked()
{
    if(ui->lineEditInputCity->text().isEmpty())
        return;
    int autoUpdate = mConfigInfoTool->getConfigInfo("AutoUpdate").toInt();
    if(0 == autoUpdate) //开启自动更新天气才去更新
        return;
    getCityWeather(ui->lineEditInputCity->text());//查询城市
    ui->lineEditInputCity->clear();
}

//左按钮被点击
void WeatherForecast::on_btnLeftSelect_clicked()
{
    if(!mBtnCanOp)
        return;
    btnUpDateUI("左按钮");
    ui->tempWidget->update();//产生温度曲线图更新时间
}

//右按钮被点击
void WeatherForecast::on_btnRightSelect_clicked()
{
    if(!mBtnCanOp)
        return;
    btnUpDateUI("右按钮");
    ui->tempWidget->update();//产生温度曲线图更新时间
}

//读文件按钮被点击
void WeatherForecast::on_btnRightRead_clicked()
{
    QFile file(mConfigInfoTool->getMemoPath());
    if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QString memoStr = file.readAll();
        if(memoStr.isEmpty())
            return;
        if(ui->plainTextMemo->toPlainText().isEmpty())
            ui->plainTextMemo->setPlainText(memoStr);
        else{ //保存原来的数据
            memoStr = ui->plainTextMemo->toPlainText()
                    + "\n<------------👇已存数据👇------------>\n" + memoStr;
            ui->plainTextMemo->setPlainText(memoStr);
        }
        file.close();
        return;
    }
    //启用备份文件
    QFile backUpFile(mConfigInfoTool->getBackUpPath());
    if(backUpFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        QString memoStr = backUpFile.readAll();
        if(memoStr.isEmpty())
            return;
        if(ui->plainTextMemo->toPlainText().isEmpty())
            ui->plainTextMemo->setPlainText(memoStr);
        else{ //保存原来的数据
            memoStr = ui->plainTextMemo->toPlainText()
                    + "\n<------------👇已存数据👇------------>\n" + memoStr;
            ui->plainTextMemo->setPlainText(memoStr);
        }
        backUpFile.close();
    }
}

//定时器保存文件
void WeatherForecast::onTimeOutSaveFile()
{
    //memo窗口模式情况下主窗口不允许对文件进行写操作
    if(Q_NULLPTR != mMemoWindow)
        return;

    int autoSaveFile = mConfigInfoTool->getConfigInfo("AutoSaveFile").toInt();
    if(0 == autoSaveFile) //不自动保存
        return;

    int fileSaveTime = mConfigInfoTool->getConfigInfo("FileSaveTime_min").toInt();//自动保存文件时间
    int autoUpdateTime = mConfigInfoTool->getConfigInfo("WeatherAutoUpdateTime_min").toInt();//自动更新天气时间

    static int mainSaveFileFlag = 1; //此时定时器已经执行了一个周期
    int temp = fileSaveTime/autoUpdateTime;
    if( temp != mainSaveFileFlag){ //时间没到
        ++mainSaveFileFlag;
        return;
    }
    mainSaveFileFlag = 1;
    on_btnLeftSave_clicked();
}

/***********************************************************************
 * @brief:  保存文件
 * @note:  memo窗口模式情况下主窗口不允许对文件进行写操作
 ***********************************************************************/
void WeatherForecast::on_btnLeftSave_clicked()
{
    QString memoStr = ui->plainTextMemo->toPlainText();
    if(memoStr.isEmpty()) //没有内容可写
        return;

    int isFileJunk = mConfigInfoTool->getConfigInfo("FileJunk").toInt();
    if(1 == isFileJunk)//废弃内容，直接返回不写入
        return;

    //创建文件
    QFile file(mConfigInfoTool->getMemoPath());
    QFile backUpFile(mConfigInfoTool->getBackUpPath());
    QByteArray byteArray = memoStr.toUtf8(); //memo文件内容
    int isFileCover = mConfigInfoTool->getConfigInfo("FileCover").toInt();
    int isFileAppend = mConfigInfoTool->getConfigInfo("FileAppend").toInt();

    //截断写（覆盖写）
    if(1 == isFileCover){
        //写入memo文件
        if(file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)){
            file.write(byteArray,byteArray.length());
            file.close();
        }
        //写入备份文件
        if(backUpFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)){
            backUpFile.write(byteArray,byteArray.length());
            backUpFile.close();
        }
        return;
    }
    //追加写
    if(1 == isFileAppend){
        //写入memo文件
        if(file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append)){
            file.write(byteArray,byteArray.length());
            file.close();
        }
        //写入备份文件
        if(backUpFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append)){
            backUpFile.write(byteArray,byteArray.length());
            backUpFile.close();
        }
    }
}

//搜索城市名称
void WeatherForecast::on_btnWeatherCity_clicked()
{
    //查询城市，优先百度搜索
    if(QDesktopServices::openUrl(QUrl(mConfigInfoTool->getBaiDuUrl() + ui->btnWeatherCity->text())))
        return;
    if(QDesktopServices::openUrl(QUrl(mConfigInfoTool->getBaiDuUrl() + ui->btnWeatherCity->text())))
        return;
    if(QDesktopServices::openUrl(QUrl(mConfigInfoTool->get360Url()  + ui->btnWeatherCity->text())))
        return;
}

//关闭按钮被点击
void WeatherForecast::on_btnClose_clicked()
{
    int closeExit = mConfigInfoTool->getConfigInfo("CloseExit").toInt();
    if(0 == closeExit){
        closeRightMenu();
        this->close();
    }
    else if(1 == closeExit) //开启缩小窗口退出程序
        qApp->quit();//退出程序
}

//设置按钮被点击
void WeatherForecast::on_btnSettings_clicked()
{
    //配置窗口
    if(Q_NULLPTR == mConfigInfo){
        mConfigInfo = new ConfigInfo(this);//设置窗口
        mConfigInfo->setAttribute(Qt::WA_DeleteOnClose);//设置窗口关闭自动释放内存
        mConfigInfo->setWindowFlag(Qt::WindowStaysOnTopHint);
        mConfigInfo->move((this->width() - mConfigInfo->width())/2,(this->height() - mConfigInfo->height())/2 - 20);
        connect(mConfigInfo,SIGNAL(configInfoSync(int,int,int,int,int,int,int,QString,QString)),
                mConfigInfoTool,SLOT(onConfigInfoSync(int,int,int,int,int,int,int,QString,QString)));//连接同步数据
        connect(mConfigInfo,SIGNAL(configInfoClose()),this,SLOT(onConfigInfoClose())); //允许创建设置窗口
        connect(mConfigInfo,SIGNAL(configInfoClose()),this,SLOT(onCreateMenus()));//开启右键小窗口选择
        connect(mConfigInfo,SIGNAL(cityChange(QString)),this,SLOT(onCityChange(QString)));
        mConfigInfo->show();
    }
    //置回原位
    if(Q_NULLPTR != mConfigInfo)
        mConfigInfo->move((this->width() - mConfigInfo->width())/2,(this->height() - mConfigInfo->height())/2 - 20);
}

//配置窗口关闭
void WeatherForecast::onConfigInfoClose() { mConfigInfo = Q_NULLPTR; }

//创建天气小窗口
void WeatherForecast::onCreateMidWindow()
{
    if(Q_NULLPTR != mMidWindow)
        return;

    mMidWindow = new MidWeather;//已在WeatherForecast析构函数中判断释放内存
    mMidWindow->setAttribute(Qt::WA_DeleteOnClose);//设置窗口关闭自动释放内存
    mMidWindow->setWindowFlags( Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint | Qt::Tool);
    mMidWindow->setFixedSize(mMidWindow->width(),mMidWindow->height()); // 设置固定窗口大小
    mMidWindow->setAttribute(Qt::WA_TranslucentBackground);//设置窗口背景透明

    //发送天气图片和天气当前索引值
    connect(this,SIGNAL(weatherForecastData(QMap<QString,QMap<QString,QString> >&,int)),
            mMidWindow,SLOT(onWeatherForecastData(QMap<QString,QMap<QString,QString> >&,int)));
    //允许天气小窗口创建
    connect(mMidWindow,SIGNAL(midWeatherClose()),this,SLOT(onMidWeatherClose()));
    //更新天气
    connect(mSaveAndUpdateTimer,SIGNAL(timeout()),mMidWindow,SLOT(onMidRefresh()));
    //更新时间
    connect(mTimer,SIGNAL(timeout()),mMidWindow,SLOT(onMidTimeUpdate()));
    //给小窗口部分重要数据
    emit weatherForecastData(mSelectDayOrNightImage,mCurrWeatherDateIndex);

    //拿到初始位置
    int ax = mConfigInfoTool->getConfigInfo("Min-XPos").toInt();
    int ay = mConfigInfoTool->getConfigInfo("Min-YPos").toInt();
    this->close();//关闭主窗口
    mMidWindow->move(ax,ay);
    mMidWindow->show();
}

//天气小窗口关闭
void WeatherForecast::onMidWeatherClose()
{
    mWinIsTop = false;//允许窗口置顶
    mMidWindow = Q_NULLPTR;//置空

    //重置位置，这里搭配第一次不显示主窗口情况下
    int ax = mConfigInfoTool->getConfigInfo("Max-XPos").toInt();
    int ay = mConfigInfoTool->getConfigInfo("Max-YPos").toInt();
    this->move(ax, ay);
    this->show();

    closeRightMenu();//关闭右键菜单
    createRightMenu();//创建右键菜单
    int autoUpdate = mConfigInfoTool->getConfigInfo("AutoUpdate").toInt();//自动更新
    if(1 == autoUpdate)
        this->getCityWeather(mConfigInfoTool->getConfigInfo("City"));//立刻刷新主窗口数据
}

//创建Memo窗口
void WeatherForecast::onCreateMemoWindow()
{
    if(Q_NULLPTR != mMemoWindow)
        return;

    mMemoWindow = new Memo;//已在WeatherForecast析构函数中判断释放内存
    mMemoWindow->setAttribute(Qt::WA_DeleteOnClose);//设置窗口关闭自动释放内存!!!!!!
    mMemoWindow->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint | Qt::Tool);// Qt::WindowStaysOnTopHint
    mMemoWindow->setFixedSize(mMemoWindow->width(),mMemoWindow->height()); // 设置固定窗口大小
    mMemoWindow->setAttribute(Qt::WA_TranslucentBackground);//设置窗口背景透明

    //允许窗口创建
    connect(mMemoWindow,SIGNAL(memoClose()),this,SLOT(onMemoClose()));
    //保存文件
    connect(mSaveAndUpdateTimer,SIGNAL(timeout()),mMemoWindow,SLOT(onTimeOutSaveFile()));
    connect(this,SIGNAL(maindataToMemoData(QString &)),mMemoWindow,SLOT(onMaindataToMemoData(QString &)));
    connect(mMemoWindow,SIGNAL(memoDataToMaindata(QString)),this,SLOT(onMemoDataToMaindata(QString)));
    //发送主窗口的数据
    QString memoStr = ui->plainTextMemo->toPlainText();
    if(!memoStr.isEmpty()){
        emit maindataToMemoData(memoStr);
        ui->plainTextMemo->clear();//清除原有数据
    }

    //拿到初始位置
    int ax = mConfigInfoTool->getConfigInfo("Memo-XPos").toInt();
    int ay = mConfigInfoTool->getConfigInfo("Memo-YPos").toInt();
    this->close();
    mMemoWindow->move(ax,ay);
    mMemoWindow->show();
}

//Memo窗口关闭
void WeatherForecast::onMemoClose()
{
    mWinIsTop = false;//允许窗口置顶
    mMemoWindow = Q_NULLPTR;//置空

    //重置位置，这里搭配第一次不显示主窗口情况下
    int ax = mConfigInfoTool->getConfigInfo("Max-XPos").toInt();
    int ay = mConfigInfoTool->getConfigInfo("Max-YPos").toInt();\
    closeRightMenu();//关闭右键菜单
    createRightMenu();//创建右键菜单
    this->move(ax,ay);
    this->show();

    int autoUpdate = mConfigInfoTool->getConfigInfo("AutoUpdate").toInt();//自动更新
    if(1 == autoUpdate)
        this->getCityWeather(mConfigInfoTool->getConfigInfo("City"));//立刻刷新主窗口数据
}

//配置窗口关闭，城市名称不同，需要更新UI
void WeatherForecast::onCityChange(QString newCity)
{
    int autoUpdate = mConfigInfoTool->getConfigInfo("AutoUpdate").toInt();
    if(0 == autoUpdate) //开启自动更新天气才去更新
        return;
    getCityWeather(newCity);
}

//天气更新
void WeatherForecast::onTimeOutUpdateWeather()
{
    //小天气窗口，memo窗口下不更新主窗口天气
    if(Q_NULLPTR != mMidWindow || Q_NULLPTR != mMemoWindow)
        return;

    int autoUpdate = mConfigInfoTool->getConfigInfo("AutoUpdate").toInt();
    if(0 == autoUpdate) //开启自动更新天气才去更新
        return;
    getCityWeather(ui->btnWeatherCity->text());
}

//接收memo窗口发送过来的数据
void WeatherForecast::onMemoDataToMaindata(const QString memoStr)
{
    ui->plainTextMemo->setPlainText(memoStr);
}

//重写右键菜单，实现右键菜单事件
void WeatherForecast::contextMenuEvent(QContextMenuEvent *event)
{
    mRightKeyMenu->exec(QCursor::pos());//弹出右键菜单
    event->accept();
}

//重写重绘函数，实现窗口圆角
void WeatherForecast::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing,true);
    painter.setBrush(QColor(235, 232, 229));// Qt::white  QColor(178,187,190) rgb(235, 232, 229)
    painter.setPen(Qt::transparent);//透明
    QRect rect = this->rect();
    rect.setWidth(rect.width() - 1);
    rect.setHeight(rect.height() - 1);
    painter.drawRoundedRect(rect, 15, 15); //rect:绘制区域
    QMainWindow::paintEvent(event);
}

//窗口拖动：鼠标移动事件
void WeatherForecast::mouseMoveEvent(QMouseEvent *event)
{
    //    QPoint y = event->globalPos(); // 鼠标相对于桌面左上角位置
    //    QPoint x = y - this->mZOffset;
    //    this->move(x);
    if(mZOffset == QPoint()) //防止按钮拖动导致的奇怪问题
        return;

    int isDrag = mConfigInfoTool->getConfigInfo("Max-IsDrag").toInt();
    if(1 == isDrag)
        this->move(event->globalPos() - mZOffset);
    QMainWindow::mouseMoveEvent(event);
}

//窗口拖动：鼠标按压事件
void WeatherForecast::mousePressEvent(QMouseEvent *event)
{
    //    QPoint x = this->geometry().topLeft(); // 窗口左上角相对于桌面左上角位置
    //    QPoint y = event->globalPos(); // 鼠标相对于桌面左上角位置
    //    this->mZOffset = y - x; // 这是个定值
    this->setCursor(Qt::ClosedHandCursor);
    this->mZOffset = event->globalPos() - this->pos();
    QMainWindow::mousePressEvent(event);
}

//窗口拖动：鼠标释放事件
void WeatherForecast::mouseReleaseEvent(QMouseEvent *event)
{
    this->setCursor(Qt::ArrowCursor);
    this->mZOffset = QPoint();
    QMainWindow::mouseReleaseEvent(event);
}

//初始化天气map
void WeatherForecast::addMapWeatherImage()
{
    //白天
    mDayWeatherImage.insert("暴雪",":/images/res/weather/day/暴雪.gif");
    mDayWeatherImage.insert("暴雨",":/images/res/weather/day/暴雨.gif");
    mDayWeatherImage.insert("暴雨到大暴雨",":/images/res/weather/day/暴雨到大暴雨.gif");
    mDayWeatherImage.insert("大暴雨",":/images/res/weather/day/大暴雨.gif");
    mDayWeatherImage.insert("大暴雨到特大暴雨",":/images/res/weather/day/大暴雨到特大暴雨.gif");
    mDayWeatherImage.insert("大到暴雪",":/images/res/weather/day/大到暴雪.gif");
    mDayWeatherImage.insert("大雪",":/images/res/weather/day/大雪.gif");
    mDayWeatherImage.insert("大雨",":/images/res/weather/day/大雨.gif");
    mDayWeatherImage.insert("冻雨",":/images/res/weather/day/冻雨.gif");
    mDayWeatherImage.insert("多云",":/images/res/weather/day/多云.gif");
    mDayWeatherImage.insert("浮沉",":/images/res/weather/day/浮尘.gif");
    mDayWeatherImage.insert("雷阵雨",":/images/res/weather/day/雷阵雨.gif");
    mDayWeatherImage.insert("雷阵雨伴有冰雹",":/images/res/weather/day/雷阵雨伴有冰雹.gif");
    mDayWeatherImage.insert("霾",":/images/res/weather/day/霾.png");
    mDayWeatherImage.insert("强沙尘暴",":/images/res/weather/day/强沙尘暴.gif");
    mDayWeatherImage.insert("晴",":/images/res/weather/day/晴.gif");
    mDayWeatherImage.insert("沙尘暴",":/images/res/weather/day/沙尘暴.gif");
    mDayWeatherImage.insert("特大沙尘暴",":/images/res/weather/day/特大暴雨.gif");
    mDayWeatherImage.insert("雾",":/images/res/weather/day/雾.gif");
    mDayWeatherImage.insert("小到中雨",":/images/res/weather/day/小大中雨.gif");
    mDayWeatherImage.insert("小到中雪",":/images/res/weather/day/小到中雪.gif");
    mDayWeatherImage.insert("小雪",":/images/res/weather/day/小雪.gif");
    mDayWeatherImage.insert("小雨",":/images/res/weather/day/小雨.gif");
    mDayWeatherImage.insert("雪",":/images/res/weather/day/雪.gif");
    mDayWeatherImage.insert("扬沙",":/images/res/weather/day/扬沙.gif");
    mDayWeatherImage.insert("阴",":/images/res/weather/day/阴.gif");
    mDayWeatherImage.insert("雨",":/images/res/weather/day/雨.gif");
    mDayWeatherImage.insert("雨夹雪",":/images/res/weather/day/雨夹雪.gif");
    mDayWeatherImage.insert("阵雪",":/images/res/weather/day/阵雪.gif");
    mDayWeatherImage.insert("中到大雪",":/images/res/weather/day/中到大雪.gif");
    mDayWeatherImage.insert("中到大雨",":/images/res/weather/day/中到大雨.gif");
    mDayWeatherImage.insert("中雪",":/images/res/weather/day/中雪.gif");
    mDayWeatherImage.insert("中雨",":/images/res/weather/day/中雨.gif");

    //晚上
    mNightWeatherImage.insert("暴雪",":/images/res/weather/night/暴雪.gif");
    mNightWeatherImage.insert("暴雨",":/images/res/weather/night/暴雨.gif");
    mNightWeatherImage.insert("暴雨到大暴雨",":/images/res/weather/night/暴雨到大暴雨.gif");
    mNightWeatherImage.insert("大暴雨",":/images/res/weather/night/大暴雨.gif");
    mNightWeatherImage.insert("大暴雨到特大暴雨",":/images/res/weather/night/大暴雨到特大暴雨.gif");
    mNightWeatherImage.insert("大到暴雪",":/images/res/weather/night/大到暴雪.gif");
    mNightWeatherImage.insert("大雪",":/images/res/weather/night/大雪.gif");
    mNightWeatherImage.insert("大雨",":/images/res/weather/night/大雨.gif");
    mNightWeatherImage.insert("冻雨",":/images/res/weather/night/冻雨.gif");
    mNightWeatherImage.insert("多云",":/images/res/weather/night/多云.gif");
    mNightWeatherImage.insert("浮沉",":/images/res/weather/night/浮尘.gif");
    mNightWeatherImage.insert("雷阵雨",":/images/res/weather/night/雷阵雨.gif");
    mNightWeatherImage.insert("雷阵雨伴有冰雹",":/images/res/weather/night/雷阵雨伴有冰雹.gif");
    mNightWeatherImage.insert("霾",":/images/res/weather/night/霾.png");
    mNightWeatherImage.insert("强沙尘暴",":/images/res/weather/night/强沙尘暴.gif");
    mNightWeatherImage.insert("晴",":/images/res/weather/night/晴.gif");
    mNightWeatherImage.insert("沙尘暴",":/images/res/weather/night/沙尘暴.gif");
    mNightWeatherImage.insert("特大沙尘暴",":/images/res/weather/night/特大暴雨.gif");
    mNightWeatherImage.insert("雾",":/images/res/weather/night/雾.gif");
    mNightWeatherImage.insert("小到中雨",":/images/res/weather/night/小大中雨.gif");
    mNightWeatherImage.insert("小到中雪",":/images/res/weather/night/小到中雪.gif");
    mNightWeatherImage.insert("小雪",":/images/res/weather/night/小雪.gif");
    mNightWeatherImage.insert("小雨",":/images/res/weather/night/小雨.gif");
    mNightWeatherImage.insert("雪",":/images/res/weather/night/雪.gif");
    mNightWeatherImage.insert("扬沙",":/images/res/weather/night/扬沙.gif");
    mNightWeatherImage.insert("阴",":/images/res/weather/night/阴.gif");
    mNightWeatherImage.insert("雨",":/images/res/weather/night/雨.gif");
    mNightWeatherImage.insert("雨夹雪",":/images/res/weather/night/雨夹雪.gif");
    mNightWeatherImage.insert("阵雪",":/images/res/weather/night/阵雪.gif");
    mNightWeatherImage.insert("中到大雪",":/images/res/weather/night/中到大雪.gif");
    mNightWeatherImage.insert("中到大雨",":/images/res/weather/night/中到大雨.gif");
    mNightWeatherImage.insert("中雪",":/images/res/weather/night/中雪.gif");
    mNightWeatherImage.insert("中雨",":/images/res/weather/night/中雨.gif");

    mSelectDayOrNightImage.insert("白天",mDayWeatherImage);
    mSelectDayOrNightImage.insert("晚上",mNightWeatherImage);
}
