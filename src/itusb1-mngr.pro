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
    cp2130.cpp \
    datalog.cpp \
    main.cpp \
    mainwindow.cpp \
    itusb1device.cpp \
    libusb-extra.c \
    devicewindow.cpp \
    aboutdialog.cpp \
    informationdialog.cpp \
    metrics.cpp

HEADERS += \
    cp2130.h \
    datalog.h \
    datapoint.h \
    mainwindow.h \
    itusb1device.h \
    libusb-extra.h \
    devicewindow.h \
    aboutdialog.h \
    informationdialog.h \
    metrics.h

FORMS += \
    mainwindow.ui \
    devicewindow.ui \
    aboutdialog.ui \
    informationdialog.ui

TRANSLATIONS += \
    translations/itusb1-mngr_en.ts \
    translations/itusb1-mngr_en_US.ts \
    translations/itusb1-mngr_pt.ts \
    translations/itusb1-mngr_pt_PT.ts

LIBS += -lusb-1.0

RESOURCES += \
    resources.qrc

# Added installation option
unix {
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }
    target.path = $$PREFIX/bin
    icon.files += icons/itusb1-mngr.png
    icon.path = $$PREFIX/share/icons/hicolor/128x128/apps
    shortcut.files = misc/itusb1-mngr.desktop
    shortcut.path = $$PREFIX/share/applications
    INSTALLS += icon
    INSTALLS += shortcut
}

!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    icons/itusb1-mngr.png \
    misc/itusb1-mngr.desktop
