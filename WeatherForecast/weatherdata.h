#ifndef WEATHERDATA_H
#define WEATHERDATA_H

#include <QString>

//天气数据类
class WeatherData
{
public:
    WeatherData()
    {
        province = "";
        city = "";
        humidity = "";
        sunRise = "";
        sunSet = "";
        pM25 = 0;
        airQuality = "";
        temperature = "";
        coldReminder = "";
        highTemperature = "";
        lowTemperature = "";
        date = "";
        whichWeek = "";
        windDirection = "";
        windPower = "";
        weatherType = "";
        notice = "";
    }

public:
    QString province;
    QString city;
    QString humidity;
    QString sunRise;
    QString sunSet;
    int pM25;
    QString airQuality;
    QString temperature;
    QString coldReminder;
    QString highTemperature;
    QString lowTemperature;
    QString date;
    QString whichWeek;
    QString windDirection;
    QString windPower;
    QString weatherType;
    QString notice;
};

#endif // WEATHERDATA_H




