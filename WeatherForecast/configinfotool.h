#ifndef CONFIGINFOTOOL_H
#define CONFIGINFOTOOL_H

#include <QObject>
#include <QSettings>
#include <QFile>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>

#define mem QCoreApplication::applicationDirPath()+ "/data/memo.txt"

/***********************************************************************
 * @brief: 配置类，操作配置文件保存一些资源信息:
 ***********************************************************************/
class ConfigInfoTool : public QObject
{
    Q_OBJECT

public:
//    static ConfigInfoTool* getInstance(){ return pConfigInfoTool;}
    QString getConfigInfo(const QString key);//获取配置信息
    QString getCurrYearZodiac();//获取当前年生肖
    bool setConfigInfo(const QString key,const QString value);//更新配置信息

    QString getMemoPath(){return mMemoPath;}
    QString getBackUpPath(){return mbackMemoPath;}
    QString getTempWeatherPath(){return mTempWeatherPath;}
    QString getBingUrl(){return mBingUrl;}
    QString getBaiDuUrl(){return mBaiDuUrl;}
    QString get360Url(){return m360Url;}
    QString getRequestUrl(){return mUpYunRequestUrl;}

    explicit ConfigInfoTool(QObject *parent = nullptr);

public slots:
    void onConfigInfoSync(int autoSaveFile,int closeExit,int smallWindow,int autoUpdate,
                          int fileCover,int fileAppend,int fileJunk,QString winModel,QString city);

private:
    //    static ConfigInfoTool* pConfigInfoTool;
    void iniMember();//初始化成员变量
    void iniConfigFile();//初始化配置文件
    void iniMap();//初始化map

    QMap<QString,QString> mConfigInfo;//配置信息
    QMap<QString,QString>::iterator it;

    QString mMemoPath;//memo文件路径
    QString mbackMemoPath;//备份文件路径
    QString mTempWeatherPath;//临时天气文件路径

    QString mBingUrl;  //查询天气链接
    QString mBaiDuUrl;
    QString m360Url;

    QString mUpYunRequestUrl; //网络请求链接
    QString mHeFengRequestUrl;
    QString mGaoDeRequestUrl;
    QString mXinZhiRequestUrl;
    QString mDocs;
};

//ConfigInfoTool* ConfigInfoTool::pConfigInfoTool = new ConfigInfoTool;

#endif // CONFIGINFOTOOL_H
