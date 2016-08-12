#-------------------------------------------------
#
# Project created by QtCreator 2016-07-26T11:33:54
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = DownloadTool
TEMPLATE = app

LIBS += "D:\Qt\5.5\mingw492_32\bin\libcurl.dll"
LIBS += -L D:\Python3.5.2\libs -l python3

INCLUDEPATH += -I D:\Python3.5.2\include
INCLUDEPATH += "E:\curl\curl-7.50.1\curl-7.50.1\include"
INCLUDEPATH += "E:\curl\curl-7.50.1\curl-7.50.1\include\curl"


SOURCES += main.cpp\
    download.cpp \
    ftptask.cpp \
    taskmanager.cpp \
    taskbase.cpp \
    helloworld.cpp \
    httptask.cpp

HEADERS  += \
    download.h \
    ftptask.h \
    taskmanager.h \
    taskbase.h \
    helloworld.h \
    httptask.h

FORMS += \
    helloworld.ui

DISTFILES += \
    getsize.py

RESOURCES += \
    resourse.qrc
