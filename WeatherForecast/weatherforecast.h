#ifndef WEATHERFORECAST_H
#define WEATHERFORECAST_H

#include "weatherdata.h"
#include "cityinfotool.h"
#include "configinfo.h"
#include "midweather.h"
#include "memo.h"
#include "configinfotool.h"

#include "windows.h"
#include <QMainWindow>
#include <QContextMenuEvent>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QEvent>
#include <QPlainTextEdit>
#include <QNetworkAccessManager>//网络访问管理
#include <QNetworkRequest>//网络请求
#include <QNetworkReply>//网络回应
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QMap>
#include <QSystemTrayIcon>
#include <QCommonStyle>
#include <QMessageBox>
#include <QDesktopServices>
#include <QTimer>
#include <QDateTime>
#include <QSettings>
#include <QCoreApplication>
#include <QThread>

//温度线绘制配置
#define TEMP_INCREASE_PIXEL 1.5  //1°C对应的像素的增长量
#define TEMP_POINT_SIZE 3       //温度点大小
#define TEMP_HIGH_TEXT_X_OFFSET 4   //高温 X轴的偏移量
#define TEMP_HIGH_TEXT_Y_OFFSET 6   //高温 Y轴偏移量
#define TEMP_LOW_TEXT_X_OFFSET 4   //低温 X轴的偏移量
#define TEMP_LOW_TEXT_Y_OFFSET 16   //低温 Y轴偏移量
#define TEMP_HIGH_Y_OFFSET 22   //高温线向下偏移量
#define TEMP_LOW_Y_OFFSET 22    //低温线向上偏移量

namespace Ui {
class WeatherForecast;
}

class WeatherForecast : public QMainWindow
{
    Q_OBJECT

public:
    explicit WeatherForecast(QWidget *parent = 0);
    ~WeatherForecast();

    void upDateUI();//更新UI

signals:
    void weatherForecastData(QMap<QString,QMap<QString,QString>> &selectDayOrNightImage,int currWeatherDateIndex);

    void midWinReset();//复位窗口

    void memoWinReset();//复位窗口

    void mainClose();//通知memo窗口保存数据

    void maindataToMemoData(QString & memoStr);//主窗口的memo内容给memo窗口

private slots:
    void onReply(QNetworkReply *reply);//响应请求
    void onTiggered(QAction *action);//系统托盘选项被点击操作
    void onRefresh();//刷新操作
    void onActivated(QSystemTrayIcon::ActivationReason reason);//系统托盘图标被点击操作
    void onTimeUpdate();//更新时间
    void onCreateMenus();//创建右键菜单选项
    void onConfigInfoClose();//配置窗口关闭
    void onCreateMemoWindow();//创建Memo窗口
    void onMemoClose();//Memo窗口关闭
    void onCityChange(QString newCity);//设置窗口城市名称改变，需要更新UI
    void onCreateMidWindow();//创建天气小窗口
    void onMidWeatherClose();//天气小窗口关闭
    void onTimeOutUpdateWeather();//自动更新天气
    void onTimeOutSaveFile();//定时器保存文件
    void onMemoDataToMaindata(const QString memoStr);//接收memo窗口发送过来的数据
    void on_btnSearch_clicked();//搜索
    void on_btnLeftSelect_clicked();
    void on_btnRightSelect_clicked();
    void on_btnWeatherCity_clicked();//天气城市按钮
    void on_btnSettings_clicked();//创建配置窗口
    void on_btnClose_clicked();
    void on_btnLeftSave_clicked();//保存文件
    void on_btnRightRead_clicked();//读文件

protected:
    virtual void contextMenuEvent(QContextMenuEvent *event) override;//右键菜单
    virtual void paintEvent(QPaintEvent *event) override;
    virtual bool eventFilter(QObject* watched,QEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Ui::WeatherForecast *ui;

private:
    void iniMember();//初始化成员变量
    void btnUpDateUI(const QString & upDateType);//更新UI
    void painTemperatureCurve();//绘制温度曲线
    void addDateWeekLabelToList();//将Label添加至List管理
    void addMapWeatherImage();//添加天气类型对应的图片
    void createRightMenu();//创建右键菜单
    void closeRightMenu();//关闭右键菜单
    void createystemTrayIcon();//创建系统托盘
    void getCityWeather(const QString & cityName);//通过城市ID获取天气信息
    bool parseUpYunJson(const QByteArray & byteArray);//解析UpYun获取到的天气信息

    int mCurrWeatherDateIndex;//当前天气索引，默认是当天 0
    WeatherData mWeatherData[7];//七天的天气数据

    QTimer *mTimer; //时间显示定时器，主窗口与天气小窗口共用，1s
    QTimer *mSaveAndUpdateTimer; //文件保存,天气更新定时器，主窗口与memo小窗口共用
    QThread *mTimerThread;

    QList<QLabel*> mBottomDate;//七天的日期
    QList<QLabel*> mBottomWeek;//七天的星期
    QMap<QString,QString> mDayWeatherImage;//白天天气类型对应的图片
    QMap<QString,QString> mNightWeatherImage;//晚上天气类型对应的图片
    QMap<QString,QMap<QString,QString>> mSelectDayOrNightImage;

    CityInfoTool *mCityInfoTool;//天气工具类
    ConfigInfoTool *mConfigInfoTool ;//配置信息工具类
    QNetworkAccessManager *mNetworkAccessManager;//网络访问管理

    QSystemTrayIcon *mSystemTrayIcon;//系统托盘
    QMenu *mSystemTrayMenu;// 右键退出的菜单
    QPoint mZOffset;//z的偏移值

    QMenu *mRightKeyMenu;// 右键退菜单
    QAction *mActRefresh;// 键菜单-刷新
    QAction *mActMidWindow;//右键菜单-小天气窗口
    QAction *mActMemoWindow;//右键菜单-memo窗口

    bool mWinIsTop;//小天气窗口mMemo窗口置顶置底标识符，默认置底状态 false
    bool mBtnCanOp;//默认按钮可操作 true，避免数据拿取越界

    ConfigInfo *mConfigInfo;//设置窗口
    MidWeather *mMidWindow;//中间小窗口
    Memo *mMemoWindow;//备忘录窗口
};

#endif // WEATHERFORECAST_H
