#include "configinfotool.h"

ConfigInfoTool::ConfigInfoTool(QObject *parent) : QObject(parent)
{
    iniMember();//初始化成员变量
    iniConfigFile();//初始化配置文件
    iniMap();//初始化map
}

void ConfigInfoTool::iniMember()
{
    //数据存放地，直接写死
    mMemoPath = mem;
    mbackMemoPath = QCoreApplication::applicationDirPath()+ "/backup/memo.txt";
    mTempWeatherPath = QCoreApplication::applicationDirPath()+ "/resources/temp_weather_forecast.json";

    //搜索链接
    mBingUrl="https://cn.bing.com/search?ie=UTF-8&q=";
    mBaiDuUrl="http://www.baidu.com/s?ie=UTF-8&wd=";
    m360Url="https://www.so.com/s?ie=utf8&q=";

    //天气请求连接
    mUpYunRequestUrl = "http://t.weather.itboy.net/api/weather/city/";
    mHeFengRequestUrl = "https://devapi.qweather.com/v7/weather/now?&key=3dbc6873aea346d1bb91eb6e17d0dfc5&"
                        "location=";
    mXinZhiRequestUrl = "https://api.seniverse.com/v3/weather/now.json?key=P19dyxd_x4XKAlc-X&"
                        "&language=zh-Hans&unit=clocation=";
//    mDocs = "https://api.caiyunapp.com/v2/TAkhjf8d1nlSlspN/119.45,32.2/daily?dailysteps=7";
//    mHeFengRequestUrl = "https://api.qweather.com/v7/weather/7d?[请求参数]";
    mGaoDeRequestUrl = "https://restapi.amap.com/v3/weather/weatherInfo?key=5d1890b25e21a89b898287ce292d5100&"
                       "extensions=base&city=";
}

//获取配置信息
//重要！！！！
QString ConfigInfoTool::getConfigInfo(const QString key)
{
    it = mConfigInfo.find(key);//查找
    if(it != mConfigInfo.end())
        return it.value();
    return "";
}

/***********************************************************************
 * @brief: 拿到当年属于什么年
 * @param:
 * @note: 根据配置文件的推断
 *      鼠 牛 虎 兔 龙 蛇 马 羊 猴 鸡 狗 猪
 ***********************************************************************/
QString ConfigInfoTool::getCurrYearZodiac()
{
    int year = getConfigInfo("Year").toInt();
    QString zodiac = getConfigInfo("Zodiac");
    int grade = getConfigInfo("Grade").toInt();

    int currYear = QDateTime::currentDateTime().toString("yyyy").toInt();
    if(currYear == year)
        return zodiac.at(grade - 1);

    //新的一年
    if((currYear - 1) == year){
        if(12 == grade) //注意最后一年
            grade = 0;
        setConfigInfo("Grade",QString::number(grade + 1));//写入新索引
        return zodiac.at(grade);
    }
    return "";
}

/***********************************************************************
 * @brief: 设置配置信息，更改生肖索引时用到
 * @param: const QString& key, const QString& value
 ***********************************************************************/
bool ConfigInfoTool::setConfigInfo(const QString key, const QString value)
{
    it = mConfigInfo.find(key);//是正确的配置信息才行
    if(it != mConfigInfo.end()){
        mConfigInfo.insert(key,value); //同步map

        QSettings newSettings("config.ini",QSettings::IniFormat); //同步配置文件//同步配置文件
        newSettings.setIniCodec("utf-8");//解决乱码
        newSettings.beginGroup("Zodiac-Configuration");
        newSettings.setValue(key,QString(value));//设置新值
        newSettings.endGroup();
        newSettings.sync();

        return true;
    }
    return false;
}

/***********************************************************************
 * @brief: 同步设置窗口配置信息至配置文件中
 * @param: autoSaveFile,closeExit,smallWindow,autoUpdate,
 *         fileCover,fileAppend,fileJunk,winModel
 * @note: 配置窗口提供8个可更改选项数据
 ***********************************************************************/
void ConfigInfoTool::onConfigInfoSync(int autoSaveFile, int closeExit, int smallWindow,int autoUpdate,
                                      int fileCover, int fileAppend, int fileJunk, QString winModel,QString city)
{
    //同步配置文件
    QSettings newSettings("config.ini",QSettings::IniFormat);
    newSettings.setIniCodec("utf-8");//解决乱码
    newSettings.beginGroup("Basic-Configuration");
    newSettings.setValue("City",QString(city));
    newSettings.setValue("AutoSaveFile",autoSaveFile);//自动保存memo文件
    newSettings.setValue("CloseExit",closeExit);//缩小窗口退出程序
    newSettings.setValue("SmallWindow",smallWindow);//小窗口模式
    newSettings.setValue("AutoUpdate",autoUpdate);//自动更新
    newSettings.setValue("FileCover",fileCover);//默认文件读写模式是覆盖
    newSettings.setValue("FileAppend",fileAppend);//默认文件读写模式不是追加
    newSettings.setValue("FileJunk",fileJunk);//默认文件读写模式不是废弃
    newSettings.setValue("WinModel",winModel);//窗口打开模式
    newSettings.endGroup();
    newSettings.sync();//同步

    //同步map
    mConfigInfo.insert("City",city);
    mConfigInfo.insert("AutoSaveFile",QString::number(autoSaveFile));
    mConfigInfo.insert("CloseExit",QString::number(closeExit));
    mConfigInfo.insert("SmallWindow",QString::number(smallWindow));
    mConfigInfo.insert("AutoUpdate",QString::number(autoUpdate));
    mConfigInfo.insert("FileCover",QString::number(fileCover));
    mConfigInfo.insert("FileAppend",QString::number(fileAppend));
    mConfigInfo.insert("FileJunk",QString::number(fileJunk));
    mConfigInfo.insert("WinModel",winModel);
}

/***********************************************************************
 * @brief: 拿去配置文件的信息初始化map，提供窗口便捷获取数据
 * @note:
 ***********************************************************************/
void ConfigInfoTool::iniMap()
{
    QSettings getSettings("config.ini",QSettings::IniFormat);  //加载配置文件
    getSettings.setIniCodec("utf-8");// 解决乱码
    getSettings.beginGroup("Basic-Configuration");
    mConfigInfo.insert("City",getSettings.value("City").toString());
    mConfigInfo.insert("AutoSaveFile",getSettings.value("AutoSaveFile").toString());
    mConfigInfo.insert("CloseExit",getSettings.value("CloseExit").toString());
    mConfigInfo.insert("SmallWindow",getSettings.value("SmallWindow").toString());
    mConfigInfo.insert("AutoUpdate",getSettings.value("AutoUpdate").toString());
    mConfigInfo.insert("FileCover",getSettings.value("FileCover").toString());
    mConfigInfo.insert("FileAppend",getSettings.value("FileAppend").toString());
    mConfigInfo.insert("FileJunk",getSettings.value("FileJunk").toString());
    mConfigInfo.insert("WinModel",getSettings.value("WinModel").toString());
    getSettings.endGroup();

    getSettings.beginGroup("Coordinate-Configuration");
    mConfigInfo.insert("Max-XPos",getSettings.value("Max-XPos").toString());
    mConfigInfo.insert("Max-YPos",getSettings.value("Max-YPos").toString());
    mConfigInfo.insert("Min-XPos",getSettings.value("Min-XPos").toString());
    mConfigInfo.insert("Min-YPos",getSettings.value("Min-YPos").toString());
    mConfigInfo.insert("Memo-XPos",getSettings.value("Memo-XPos").toString());
    mConfigInfo.insert("Memo-YPos",getSettings.value("Memo-YPos").toString());
    getSettings.endGroup();

    getSettings.beginGroup("Zodiac-Configuration");
    mConfigInfo.insert("Year",getSettings.value("Year").toString());
    mConfigInfo.insert("Zodiac",getSettings.value("Zodiac").toString());
    mConfigInfo.insert("Grade",getSettings.value("Grade").toString());
    getSettings.endGroup();

    getSettings.beginGroup("Time-Configuration");
    mConfigInfo.insert("FileSaveTime_min",getSettings.value("FileSaveTime_min").toString()); //文件保存时间
    mConfigInfo.insert("WeatherAutoUpdateTime_min",getSettings.value("WeatherAutoUpdateTime_min").toString());//请求天气时间
    getSettings.endGroup();

    getSettings.beginGroup("WinDrag-Configuration");
    mConfigInfo.insert("Max-IsDrag",getSettings.value("Max-IsDrag").toString());//默认主窗口不可拖动
    mConfigInfo.insert("Min-IsDrag",getSettings.value("Min-IsDrag").toString());//默认小窗口可拖动
    mConfigInfo.insert("Memo-IsDrag",getSettings.value("Memo-IsDrag").toString());//默认Memo窗口可拖动
    getSettings.endGroup();
}

/***********************************************************************
 * @brief: 初始化配置文件，以及资源文件、数据文件和备份文件
 * @note: 启动时初始化,存在就不会再初始化
 ***********************************************************************/
void ConfigInfoTool::iniConfigFile()
{
    QFile configFile(mTempWeatherPath);
    //配置文件存在情况
    if(!configFile.exists()){
        QSettings settings("config.ini",QSettings::IniFormat);
        settings.setIniCodec("utf-8");// 解决乱码
        settings.beginGroup("Basic-Configuration");
        settings.setValue("City",QString("桂林"));//默认是桂林
        settings.setValue("AutoSaveFile",1);//默认自动保存文件
        settings.setValue("CloseExit",0);//默认关闭窗口不退出程序
        settings.setValue("SmallWindow",0);//默认关闭小窗口模式
        settings.setValue("AutoUpdate",1);//默认自动更新天气数据
        settings.setValue("FileCover",1);//默认文件读写模式是覆盖
        settings.setValue("FileAppend",0);//默认文件读写模式不是追加
        settings.setValue("FileJunk",0);//默认文件读写模式不是废弃
        settings.setValue("WinModel",QString("标准模式"));//默认主窗口模式
        settings.endGroup();

        settings.beginGroup("Coordinate-Configuration");
        settings.setValue("Max-XPos",1270);
        settings.setValue("Max-YPos",0);
        settings.setValue("Min-XPos",1551);
        settings.setValue("Min-YPos",0);
        settings.setValue("Memo-XPos",1571);
        settings.setValue("Memo-YPos",0);
        settings.endGroup();

        settings.beginGroup("Zodiac-Configuration");
        int currYear = QDateTime::currentDateTime().toString("yyyy").toInt();
        settings.setValue("Year",currYear);
        settings.setValue("Zodiac",QString("鼠牛虎兔龙蛇马羊猴鸡狗猪"));
        settings.setValue("Grade",3);
        settings.endGroup();

        settings.beginGroup("Time-Configuration");
        settings.setValue("Notice",QString("文件保存时间以天气更新时间的倍数计算！"));
        settings.setValue("FileSaveTime_min",15);//文件保存时间
        settings.setValue("WeatherAutoUpdateTime_min",5);//请求天气时间
        settings.endGroup();

        settings.beginGroup("WinDrag-Configuration");
        settings.setValue("Max-IsDrag",0);//默认主窗口不可拖动
        settings.setValue("Min-IsDrag",1);//默认小窗口可拖动
        settings.setValue("Memo-IsDrag",1);//默认Memo窗口可拖动
        settings.endGroup();
        settings.sync();//同步
    }

    //判断data,backup文件夹是否存在
    //判断memo.json,memo.json文件是否存在
    QFile memoFile(this->mMemoPath);//原始文件
    QFile backUpMemoFile(this->mbackMemoPath);//备份文件
    //创建路径
    QDir dataPath(QCoreApplication::applicationDirPath()+ "/data");
    if(!dataPath.exists()){
        dataPath = QCoreApplication::applicationDirPath();
        dataPath.mkdir("data");
    }
    QDir backUpPath(QCoreApplication::applicationDirPath()+ "/backup");
    if(!backUpPath.exists()){
        backUpPath = QCoreApplication::applicationDirPath();
        backUpPath.mkdir("backup");
    }
    //创建原始文件
    if(!memoFile.exists() && !backUpMemoFile.exists()){
        if(!memoFile.open(QIODevice::WriteOnly))
            memoFile.open(QIODevice::WriteOnly);//在创建一次，以免创建失败
        memoFile.close();
    }else if(!memoFile.exists() && backUpMemoFile.exists()){//从备份文件恢复文件
        if(!QFile::copy(this->mbackMemoPath,this->mMemoPath))
            QFile::copy(this->mbackMemoPath,this->mMemoPath);//在复制一次，以免复制失败
    }
    //创建备份文件
    if(!backUpMemoFile.exists() && !memoFile.exists()){//一般不会执行
        if(!backUpMemoFile.open(QIODevice::WriteOnly))
            backUpMemoFile.open(QIODevice::WriteOnly);
        backUpMemoFile.close();
    }else if(!backUpMemoFile.exists() && memoFile.exists()){
        if(!QFile::copy(this->mMemoPath,this->mbackMemoPath))
            QFile::copy(this->mMemoPath,this->mbackMemoPath);
    }

    //判断resources文件夹是否存在
    //判断memo.json文件是否存在，位置不变
    QDir resPath(QCoreApplication::applicationDirPath()+ "/resources");
    //创建目录
    if(!resPath.exists()){
        resPath = QCoreApplication::applicationDirPath();
        resPath.mkdir("resources");
    }
    //创建文件
    QFile resFile(this->mTempWeatherPath);
    if(!resFile.exists()){
        if(!resFile.open(QIODevice::WriteOnly))
            resFile.open(QIODevice::WriteOnly);//在创建一次，以免创建失败
        resFile.close();
    }
}
