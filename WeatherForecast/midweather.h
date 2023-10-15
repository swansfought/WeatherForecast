#ifndef MIDWEATHER_H
#define MIDWEATHER_H

#include "cityinfotool.h"
#include "weatherdata.h"
#include "configinfotool.h"

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QContextMenuEvent>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QLabel>
#include <QMessageBox>
#include <QDateTime>
#include <QTimer>
#include <QCoreApplication>
#include <QSettings>
#include <QDesktopServices>
#include <QDesktopWidget>

namespace Ui {
class MidWeather;
}

class MidWeather : public QWidget
{
    Q_OBJECT

public:
    explicit MidWeather(QWidget *parent = 0);
    ~MidWeather();

    void upDateUI();//更新UI

protected:
    virtual void contextMenuEvent(QContextMenuEvent *event) override;//右键菜单
    virtual void paintEvent(QPaintEvent *event) override;//重绘函数
    virtual void mouseMoveEvent(QMouseEvent *event) override;//移动窗口
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onReply(QNetworkReply *reply);//响应请求

    void onMidTimeUpdate();//更新时间

    void onMidRefresh();//更新天气信息

    void onWeatherForecastData(QMap<QString,QMap<QString,QString>> &selectDayOrNightImage,int currWeatherDateIndex);

    void on_memoBtnWeatherCity_clicked();

    void on_memoBtnLeftSelect_clicked();

    void on_memoBtnRightSelect_clicked();

    void on_memoBtnClose_clicked();

    void on_memoBtnSearch_clicked();

signals:
    void midWeatherClose();//窗口关闭信号

private:
    Ui::MidWeather *ui;

private:
    void iniMember();//初始化成员变量
    void btnUpDateUI(const QString & upDateType);//按钮更新UI
    void addMapWeatherImage();//添加天气类型对应的图片
    void getCityWeather(const QString & cityName);//通过城市名称获取天气信息
    bool parseUpYunJson(const QByteArray & byteArray);//解析获取到的天气信息

    //用接收主窗口过来等待数据初始化
    int mCurrWeatherDateIndex;//当前天气索引，默认是当天
    QMap<QString,QMap<QString,QString>> mSelectDayOrNightImage;
    WeatherData mWeatherData[7];//未来七天的数据

    CityInfoTool *mCityInfoTool;//天气工具类
    ConfigInfoTool *mConfigInfoTool;//配置信息管理类
    QNetworkAccessManager *mNetworkAccessManager;//网络访问管理

    QTimer *mTimer;//实现显示定时器

    bool mBtnCanOp;//默认按钮可操作，避免数据拿取越界

    QMenu *mRightKeyMenu;// 右键退出的菜单
    QAction *mActExit;// 键菜单-退出

    QPoint mZOffset;//z的偏移值
};

#endif // MIDWEATHER_H
