#include "cityinfotool.h"


CityInfoTool::CityInfoTool(QObject *parent) : QObject(parent)
{
    iniMap();
}

QString CityInfoTool::getCityCode(const QString city)
{
    //确保有数据查询
    if(mCityCode.isEmpty())
        iniMap();

    QMap<QString,QString>::iterator it = mCityCode.find(city);

    //对输入的数据再次查询处理
    //加 市 处理
    if(it == mCityCode.end())
        it = mCityCode.find(city + "市");
    //加 区 处理
    if(it == mCityCode.end())
        it = mCityCode.find(city + "区");
    //加 县 处理
    if(it == mCityCode.end())
        it = mCityCode.find(city + "县");

    //减 市 处理
    if(it == mCityCode.end() && "市" == city.right(1))
        it = mCityCode.find(city.left(city.length()-1));
    //减 区 处理
    if(it == mCityCode.end() && "区" == city.right(1))
        it = mCityCode.find(city.left(city.length()-1));
    //减 县 处理
    if(it == mCityCode.end() && "县" == city.right(1))
        it = mCityCode.find(city.left(city.length()-1));

    //终于找到
    if(it != mCityCode.end())
        return it.value();

    return "";
}

//读取JSON拿到城市名称-城市编码
void CityInfoTool::iniMap()
{
    //读取JSON数据
    QFile file(":/cityinfo/res/citycode.json");
    if(!(file.open(QIODevice::ReadOnly | QIODevice::Text)))
        return;
    QByteArray byteArray = file.readAll();
    file.close();

    //解析JSON拿到城市和对应编码
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(byteArray,&parseError);

    //注意这个JSON文件内容格式，整体是一个数组
    if(parseError.error != QJsonParseError::NoError || !doc.isArray())
        return;

    QJsonArray jsonArray = doc.array();
    QString cityName,cityCode;
    for(int i=0;i<jsonArray.size();i++){
        cityName = jsonArray[i].toObject().value("city_name").toString();
        cityCode = jsonArray[i].toObject().value("city_code").toString();

        if(!cityCode.isNull())
            mCityCode.insert(cityName,cityCode);
    }
}

