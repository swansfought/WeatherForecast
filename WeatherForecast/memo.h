#ifndef MEMO_H
#define MEMO_H

#include "configinfotool.h"

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QContextMenuEvent>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QSettings>
#include <QDesktopWidget>

namespace Ui {
class Memo;
}

class Memo : public QWidget
{
    Q_OBJECT

public:
    explicit Memo(QWidget *parent = 0);
    ~Memo();

protected:
    virtual void contextMenuEvent(QContextMenuEvent *event) override;//右键菜单
    virtual void paintEvent(QPaintEvent *event) override;//重绘函数
    virtual void mouseMoveEvent(QMouseEvent *event) override;//移动窗口
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onTiggered();//返回主窗口

    void onMaindataToMemoData(QString & memoStr);//显示主窗口发过来的memo数据

    void on_btnLeftSave_clicked();//保存文件

    void on_btnRightRead_clicked();//读取文件

    void onTimeOutSaveFile();//定时器保存文件

    void onMainClose();//程序退出需要保存数据

signals:
    void memoClose();//Memo窗口关闭

    void memoDataToMaindata(const QString memoStr);//memo窗口的memo内容给主窗口

private:
    Ui::Memo *ui;

private:
    ConfigInfoTool *mConfigInfoTool;
    QMenu *mRightKeyMenu;// 右键退出的菜单
    QAction *mActBack;// 键菜单-退出
    QPoint mZOffset;//z的偏移值

    int mFileSaveTime;//文件保存时间
    int mAutoUpdateTime;//天气自动更新时间
};

#endif // MEMO_H
