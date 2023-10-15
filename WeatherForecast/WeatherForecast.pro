#-------------------------------------------------
#
# Project created by QtCreator 2022-11-17T13:04:53
#
#-------------------------------------------------

QT       += core gui network

LIBS += -luser32

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

#
TARGET = WeatherForecast
TEMPLATE = app


RC_ICONS = Application.ico

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        weatherforecast.cpp \
    midweather.cpp \
    memo.cpp \
    configinfo.cpp \
    cityinfotool.cpp \
    configinfotool.cpp

HEADERS += \
        weatherforecast.h \
    weatherdata.h \
    midweather.h \
    memo.h \
    configinfo.h \
    cityinfotool.h \
    configinfotool.h

FORMS += \
        weatherforecast.ui \
    midweather.ui \
    memo.ui \
    configinfo.ui

RESOURCES += \
    res.qrc
