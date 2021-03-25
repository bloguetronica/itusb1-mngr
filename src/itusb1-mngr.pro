#-------------------------------------------------
#
# Project created by QtCreator 2020-05-02T16:14:46
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Added to provide backwards compatibility (C++11 support)
greaterThan(QT_MAJOR_VERSION, 4) {
    CONFIG += c++11
} else {
    QMAKE_CXXFLAGS += -std=c++11
}

TARGET = itusb1-mngr
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    itusb1device.cpp \
    libusb-extra.c \
    devicewindow.cpp \
    aboutdialog.cpp \
    informationdialog.cpp

HEADERS += \
        mainwindow.h \
    itusb1device.h \
    libusb-extra.h \
    devicewindow.h \
    aboutdialog.h \
    informationdialog.h

FORMS += \
        mainwindow.ui \
    devicewindow.ui \
    aboutdialog.ui \
    informationdialog.ui

LIBS += -lusb-1.0

RESOURCES += \
    resources.qrc
