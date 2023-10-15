#include "memo.h"
#include "ui_memo.h"

Memo::Memo(QWidget *parent) : QWidget(parent),ui(new Ui::Memo)
{
    ui->setupUi(this);

    mConfigInfoTool = new ConfigInfoTool(this);

    //添加右键菜单
    mRightKeyMenu = new QMenu(this);
    mActBack = new QAction("返回");
    mActBack->setIcon(QIcon(":/images/res/返回-正常.png"));
    mRightKeyMenu->addAction(mActBack);
    connect(mActBack,SIGNAL(triggered(bool)),this,SLOT(onTiggered()));

    //拿到自动保存文件时间
    mFileSaveTime = mConfigInfoTool->getConfigInfo("FileSaveTime_min").toInt();
    mAutoUpdateTime = mConfigInfoTool->getConfigInfo("WeatherAutoUpdateTime_min").toInt();

    on_btnRightRead_clicked();
}

Memo::~Memo()
{
    emit memoClose();
    delete ui;
}

/***********************************************************************
 * @brief: 重写右键菜单，实现右键菜单事件
 * @param: QContextMenuEvent *event
 * @note:
 ***********************************************************************/
void Memo::contextMenuEvent(QContextMenuEvent *event)
{
    mRightKeyMenu->exec(QCursor::pos());//弹出右键菜单
    event->accept();
}

//返回主窗口，并且发送数据给主窗口
void Memo::onTiggered()
{  
    QString memoStr = ui->plainTextEdit->toPlainText();
    if(!memoStr.isEmpty())
        emit memoDataToMaindata(memoStr);
    this->close();
}

//显示主窗口发过来的memo数据
void Memo::onMaindataToMemoData(QString & memoStr)
{
    ui->plainTextEdit->setPlainText(memoStr);
}

//读文件
void Memo::on_btnRightRead_clicked()
{
    QFile file(mConfigInfoTool->getMemoPath());
    if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QString memoStr = file.readAll();
        if(memoStr.isEmpty())
            return;
        if(ui->plainTextEdit->toPlainText().isEmpty())
            ui->plainTextEdit->setPlainText(memoStr);
        else{
            memoStr = ui->plainTextEdit->toPlainText()
                    + "\n<------------👇已存数据👇------------>\n" + memoStr;//保存原来的数据
            ui->plainTextEdit->setPlainText(memoStr);
        }
        file.close();
        return;
    }

    //启用备份文件
    QFile backfFle(mConfigInfoTool->getBackUpPath());
    if(backfFle.open(QIODevice::ReadOnly | QIODevice::Text)){
        QString memoStr = backfFle.readAll();
        if(memoStr.isEmpty())
            return;
        if(ui->plainTextEdit->toPlainText().isEmpty())
            ui->plainTextEdit->setPlainText(memoStr);
        else{
            memoStr = ui->plainTextEdit->toPlainText()
                    + "<------------👇已存数据👇------------>" + memoStr;//保存原来的数据
            ui->plainTextEdit->setPlainText(memoStr);
        }
        backfFle.close();
    }
}

//定时器时间到自动保存文件
void Memo::onTimeOutSaveFile()
{
    int autoSaveFile = mConfigInfoTool->getConfigInfo("AutoSaveFile").toInt();
    if(0 == autoSaveFile) //不自动保存
        return;

    int fileSaveTime = mConfigInfoTool->getConfigInfo("FileSaveTime_min").toInt();//自动保存文件时间
    int autoUpdateTime = mConfigInfoTool->getConfigInfo("WeatherAutoUpdateTime_min").toInt();//自动更新天气时间

    static int memoSaveFileFlag = 1;//此时定时器已经执行了一个周期!
    int temp = fileSaveTime/autoUpdateTime;//20/4=5min
    if( temp != memoSaveFileFlag){ //时间没到
        ++memoSaveFileFlag;
        return;
    }
    memoSaveFileFlag = 1;
    on_btnLeftSave_clicked();
}

//程序退出需要保存数据
void Memo::onMainClose() { on_btnLeftSave_clicked(); }

//保存文件
void Memo::on_btnLeftSave_clicked()
{
    //处理待写入的内容
    QString memoStr = ui->plainTextEdit->toPlainText();
    if(memoStr.isEmpty()) //没有内容可写
        return;

    int isFileJunk = mConfigInfoTool->getConfigInfo("FileJunk").toInt();
    if(1 == isFileJunk)//废弃内容，直接返回不写入
        return;

    QFile file(mConfigInfoTool->getMemoPath());
    QFile backUpFile(mConfigInfoTool->getBackUpPath());
    QByteArray byteArray = memoStr.toUtf8();
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

void Memo::paintEvent(QPaintEvent *event)
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
void Memo::mouseMoveEvent(QMouseEvent *event)
{
    if(mZOffset == QPoint()) //防止按钮拖动导致的奇怪问题
        return;

    int isDrag = mConfigInfoTool->getConfigInfo("Memo-IsDrag").toInt();
    if(1 == isDrag) //允许拖动
        this->move(event->globalPos() - mZOffset);

    QWidget::mouseMoveEvent(event);
}

//窗口拖动：鼠标按压事件
void Memo::mousePressEvent(QMouseEvent *event)
{
    this->setCursor(Qt::ClosedHandCursor);
    this->mZOffset = event->globalPos() - this->pos();
    QWidget::mousePressEvent(event);

}

//窗口拖动：鼠标释放事件
void Memo::mouseReleaseEvent(QMouseEvent *event)
{
    this->setCursor(Qt::ArrowCursor);
    mZOffset = QPoint();
    QWidget::mouseReleaseEvent(event);
}
