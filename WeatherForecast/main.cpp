#include "weatherforecast.h"

#include <QApplication>
#include <QSharedMemory>

//①将窗口嵌入到windows桌面中，尚未深入研究。与①搭配使用
static bool enumUserWindowsCB(HWND hwnd, LPARAM lParam)
{
    long wflags = GetWindowLong(hwnd, GWL_STYLE);
    if (!(wflags & WS_VISIBLE))
        return TRUE;

    HWND sndWnd;
    if (!(sndWnd=FindWindowEx(hwnd, NULL, L"SHELLDLL_DefView", NULL)))
        return TRUE;

    HWND targetWnd;
    if (!(targetWnd=FindWindowEx(sndWnd, NULL, L"SysListView32", L"FolderView")))
        return TRUE;

    HWND* resultHwnd = (HWND*)lParam;
    *resultHwnd = targetWnd;
    return FALSE;
}

//②将窗口嵌入到windows桌面中，尚未深入研究。与①搭配使用(这里更改过)
void setParentForWidget(WeatherForecast& w)
{
    HWND hdesktop = Q_NULLPTR;
    EnumWindows((WNDENUMPROC)enumUserWindowsCB, (LPARAM)&hdesktop);
    WId wId = w.winId();
    SetParent((HWND)wId, hdesktop);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //创建共享内存块
    static QSharedMemory *simpleForecast = new QSharedMemory("SimpleForecastApp");
    if (!simpleForecast->create(1)){//创建大小1b的内存
        qApp->quit(); //创建失败，说明已经有一个程序运行，退出当前程序
        return -1;
    }

    a.setQuitOnLastWindowClosed(false);//设置最后窗口退出时候退出程序

    WeatherForecast w;
    setParentForWidget(w);//设置窗口嵌入到桌面窗口中

    //根据配置文件配置相关设置
    QSettings getSettings("config.ini",QSettings::IniFormat);
    getSettings.setIniCodec("utf-8");// 解决乱码
    getSettings.beginGroup("Basic-Configuration");
    QString winModel = getSettings.value("WinModel").toString();
    getSettings.endGroup();
    getSettings.beginGroup("Coordinate-Configuration");
    int ax = getSettings.value("Max-XPos").toInt();
    int ay = getSettings.value("Max-YPos").toInt();
    getSettings.endGroup();

    if("标准模式" == winModel){
        w.move(ax,ay);
        w.show();
    }

    return a.exec();
}
