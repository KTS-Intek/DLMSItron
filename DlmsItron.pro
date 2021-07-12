#-------------------------------------------------
#
# Project created by QtCreator 2021-07-08T12:42:18
# Bohdan Zikranets bohdan@kts-intek.com
#-------------------------------------------------
QT       += core
QT       -= gui

CONFIG += c++11

TARGET = DLMSItron

TEMPLATE = lib

CONFIG         += plugin


DEFINES += DLMSITRON_LIBRARY

#for meteroperations.h
DEFINES += STANDARD_METER_PLUGIN
DEFINES += METERPLUGIN_FILETREE



# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0



include(../../../Matilda-units/meter-plugin-shared/meter-plugin-shared/meter-plugin-shared.pri)
include(../../../Matilda-units/matilda-base/type-converter/type-converter.pri)
include(../../../Matilda-units/meter-plugin-shared/DlmsProcessor/DlmsProcessor.pri)

SOURCES += dlmsitron.cpp \
    dlmsitronhelper.cpp \
    dlmsitronprocessor.cpp

HEADERS += dlmsitron.h \
    dlmsitronhelper.h \
    dlmsitronprocessor.h

EXAMPLE_FILES = zbyralko.json

linux-beagleboard-g++:{
    target.path = /opt/matilda/plugin
    INSTALLS += target
}

