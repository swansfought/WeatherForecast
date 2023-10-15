#include "configinfo.h"
#include "ui_configinfo.h"

ConfigInfo::ConfigInfo(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ConfigInfo)
{
    ui->setupUi(this);
    isSetDefaultCity = false;
    mCityInfoTool = new CityInfoTool(this);
    mConfigInfoTool = new ConfigInfoTool(this);//配置信息工具类
    iniConfigUI();//初始化配置界面
}

ConfigInfo::~ConfigInfo()
{
    emit configInfoClose();//窗口关闭
    delete ui;
}

//初始化配置界面信息
void ConfigInfo::iniConfigUI()
{
    //加载配置
    oldCity = mConfigInfoTool->getConfigInfo("City");
    int autoSaveFile = mConfigInfoTool->getConfigInfo("AutoSaveFile").toInt();
    int closeExit = mConfigInfoTool->getConfigInfo("CloseExit").toInt();
    int smallWindow = mConfigInfoTool->getConfigInfo("SmallWindow").toInt();
    int autoUpdate = mConfigInfoTool->getConfigInfo("AutoUpdate").toInt();
    int fileCover = mConfigInfoTool->getConfigInfo("FileCover").toInt();
    int fileAppend = mConfigInfoTool->getConfigInfo("FileAppend").toInt();
    int fileJunk = mConfigInfoTool->getConfigInfo("FileJunk").toInt();
    QString winModel =mConfigInfoTool->getConfigInfo("WinModel");

    if(1 == autoSaveFile)
        ui->cBoxAutoSaveMemo->setChecked(true);
    else
        ui->cBoxAutoSaveMemo->setChecked(false);
    if(1 == closeExit)
        ui->cBoxCloseExitApp->setChecked(true);
    else
        ui->cBoxCloseExitApp->setChecked(false);
    if(1 == smallWindow)
        ui->cBoxSmallWindow->setChecked(true);
    else
        ui->cBoxSmallWindow->setChecked(false);
    if(1 == autoUpdate)
        ui->cBoxAutoUpdateWeather->setChecked(true);
    else
        ui->cBoxAutoUpdateWeather->setChecked(false);

    ui->comBoxModel->setCurrentText(winModel);
    ui->lineEditDefaultCity->setText(oldCity);
    if(1 == fileCover)
        ui->radioBtnFileCover->setChecked(true);
    else
        ui->radioBtnFileCover->setChecked(false);
    if(1 == fileAppend)
        ui->radioBtnFileAppend->setChecked(true);
    else
        ui->radioBtnFileAppend->setChecked(false);
    if(1 == fileJunk)
        ui->radioBtnJunk->setChecked(true);
    else
        ui->radioBtnJunk->setChecked(false);
}

//同步配置数据至配置文件
void ConfigInfo::on_btnSure_clicked()
{
    //设置信息
    int autoSaveFile,closeExit,smallWindow,autoUpdate,fileCover,fileAppend,fileJunk;
    QString winModel,city;//窗口打开方式

    if(ui->cBoxAutoSaveMemo->isChecked())
        autoSaveFile = 1;
    else
        autoSaveFile = 0;
    if(ui->cBoxCloseExitApp->isChecked())
        closeExit = 1;
    else
        closeExit = 0;
    if(ui->cBoxSmallWindow->isChecked())
        smallWindow = 1;
    else
        smallWindow = 0;
    if(ui->cBoxAutoUpdateWeather->isChecked())
        autoUpdate = 1;
    else
        autoUpdate = 0;
    if(ui->radioBtnFileCover->isChecked())
        fileCover = 1;
    else
        fileCover = 0;
    if(ui->radioBtnFileAppend->isChecked())
        fileAppend = 1;
    else
        fileAppend = 0;
    if(ui->radioBtnJunk->isChecked())
        fileJunk = 1;
    else
        fileJunk = 0;
    winModel = ui->comBoxModel->currentText();

    city = ui->lineEditDefaultCity->text();
    if(!isSetDefaultCity){//按钮锁定，且数据不为空
        //判断是否和原来的一样，如果一样就不需要刷新
        QString cityCode = mCityInfoTool->getCityCode(city);
        QString oldCityCode =  mCityInfoTool->getCityCode(oldCity);
        if(cityCode != oldCityCode)
            emit cityChange(city);//通知主窗口需要更新
    }else
        city = mConfigInfoTool->getConfigInfo("City");//拿原来的

    //同步配置
    emit configInfoSync(autoSaveFile,closeExit,smallWindow,autoUpdate,fileCover,fileAppend,fileJunk,winModel,city);

    this->close();//关闭窗口
}

/***********************************************************************
 * @brief: 点击是否设置默认城市,注意正确城市名称才会被锁定
 * @note:
 ***********************************************************************/
void ConfigInfo::on_btnSetDefaultCity_clicked()
{
    //取消锁定
    if(!isSetDefaultCity){
        isSetDefaultCity = true;
        ui->lineEditDefaultCity->setReadOnly(false);//允许设置默认城市
    }else{//锁定输入
        //检测输入城市是否正确
        QString City = ui->lineEditDefaultCity->text();
        QString cityCode = mCityInfoTool->getCityCode(City);
        if(cityCode.isEmpty()){
            QMessageBox::information(this,"城市名称", "城市【" + City
                                     + "】不存在哦！\n请输入正确城市名称后再锁定！",QMessageBox::Ok);
            //恢复到解锁输入状态下
            isSetDefaultCity = true;
            ui->btnSetDefaultCity->setIcon(QIcon(":/images/res/解锁-黑色"));//解锁状态
            return;
        }
        isSetDefaultCity = false;
        ui->lineEditDefaultCity->setReadOnly(true);//不允许设置默认城市
    }
}

//按钮按下
void ConfigInfo::on_btnSetDefaultCity_pressed()
{
    if(!isSetDefaultCity)
        ui->btnSetDefaultCity->setIcon(QIcon(":/images/res/锁定-白色")); //锁定状态
    else
        ui->btnSetDefaultCity->setIcon(QIcon(":/images/res/解锁-白色"));//解锁状态

}

//按钮释放
void ConfigInfo::on_btnSetDefaultCity_released()
{
    if(!isSetDefaultCity)
        ui->btnSetDefaultCity->setIcon(QIcon(":/images/res/解锁-黑色")); //解锁状态
    else
        ui->btnSetDefaultCity->setIcon(QIcon(":/images/res/锁定-黑色"));//解锁状态
}

//窗口重绘
void ConfigInfo::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing,true);
    painter.setBrush(QColor(44,44,44));// Qt::white  235, 232, 229
    painter.setPen(Qt::transparent);
    QRect rect = this->rect();
    rect.setWidth(rect.width() - 1);
    rect.setHeight(rect.height() - 1);
    painter.drawRoundedRect(rect, 10, 10); //rect:绘制区域
    QWidget::paintEvent(event);
}

//窗口拖动：鼠标移动事件
void ConfigInfo::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if(mZOffset != QPoint()) //防止按钮拖动导致的奇怪问题
        this->move(event->globalPos() - mZOffset);
    //这里开启会导致父类窗口也是移动的，因此不能交付给父类再处理
    //    QWidget::mouseMoveEvent(event);
}

//窗口拖动：鼠标按压事件
void ConfigInfo::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    this->setCursor(Qt::ClosedHandCursor);
    this->mZOffset = event->globalPos() - this->pos();
}

//窗口拖动：鼠标释放事件
void ConfigInfo::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    this->setCursor(Qt::ArrowCursor);
    mZOffset = QPoint();
}


/***********************************************************************
 * @brief: 实现更改文件路径
 * @param:
 * @note: 已经启用
 ***********************************************************************/
//void ConfigInfo::on_btnSelectPath_clicked()
//{
//    //返回系统的根目录(Windows下是盘符列表)
//    QFileInfoList infoList = QDir::drives();
//    QString openPath;
//    if(0 == infoList.length())//特殊情况。尽可能其他盘，不拿C盘
//        openPath = infoList.at(0).absoluteFilePath();
//    else
//        openPath = infoList.at(1).absoluteFilePath();
//    QString newPath = QFileDialog::getExistingDirectory(this,"选择一个保存路径",openPath,QFileDialog::ShowDirsOnly);
//    ui->lineEditMemoPath->setText(newPath);
//}
