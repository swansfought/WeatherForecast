#ifndef CITYINFOTOOL_H
#define CITYINFOTOOL_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QMap>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

/***********************************************************************
 * @brief: 获取城市名称和对应的城市编码，对外提供查询城市编码
 ***********************************************************************/
class CityInfoTool : QObject
{
public:
    QString getCityCode(const QString city);//获取城市编码

    explicit CityInfoTool(QObject *parent = nullptr);
    //static CityInfoTool* getInstance(){ return pCityInfoTool; }

private:
    //static CityInfoTool* pCityInfoTool;
    void iniMap(); //初始化map
    QMap<QString,QString> mCityCode;//城市名称-城市编码
};

//CityInfoTool* CityInfoTool::pCityInfoTool = new CityInfoTool;//初始化

#endif // CITYINFOTOOL_H
