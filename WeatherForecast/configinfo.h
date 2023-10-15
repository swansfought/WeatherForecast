#ifndef CONFIGINFO_H
#define CONFIGINFO_H

#include "configinfotool.h"
#include "cityinfotool.h"

#include <QWidget>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QPainter>
#include <QSettings>
#include <QFileInfoList>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>

namespace Ui {
class ConfigInfo;
}

class ConfigInfo : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigInfo(QWidget *parent = 0);
    ~ConfigInfo();

protected:
    virtual void paintEvent(QPaintEvent *event) override;//重绘函数
    virtual void mouseMoveEvent(QMouseEvent *event) override;//移动窗口
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void configInfoClose();//窗口关闭

    void configInfoSync(int autoSaveFile,int closeExit,int smallWindow,int autoUpdate,
                    int fileCover,int fileAppend,int fileJunk,QString winModel,QString city);

    void cityChange(QString newCity);//城市名称改变

private slots:
    void on_btnSure_clicked();

    void on_btnSetDefaultCity_clicked();

    void on_btnSetDefaultCity_pressed();

    void on_btnSetDefaultCity_released();

private:
    Ui::ConfigInfo *ui;

private:
    void iniConfigUI();//初始化配置窗口UI
    bool isSetDefaultCity;//默认锁定状态不允许设置默认城市

    QString oldCity;//原来的默认城市

    CityInfoTool *mCityInfoTool;//通过查询城市编码去判断输入是否正确
    ConfigInfoTool *mConfigInfoTool;//配置信息工具类

    QPoint mZOffset;//z的偏移值
};

#endif // CONFIGINFO_H
