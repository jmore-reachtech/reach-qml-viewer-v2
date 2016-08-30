TEMPLATE = app

QT += qml quick network serialport
CONFIG += c++11

VERSION = 2.0.3
TARGET = qml-viewer
target.path=/application/bin
INSTALLS += target

# add #define for the version
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

SOURCES += main.cpp \
    maincontroller.cpp \
    mainview.cpp \
    stringserver.cpp \
    serialserver.cpp \
    translator.cpp \
    screen.cpp \
    settings.cpp \
    watchdog.cpp \
    applicationsettings.cpp

RESOURCES += \
    qt.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

HEADERS += \
    maincontroller.h \
    mainview.h \
    stringserver.h \
    systemdefs.h \
    serialserver.h \
    translator.h \
    screen.h \
    settings.h \
    watchdog.h \
    applicationsettings.h


OTHER_FILES +=

DISTFILES += \
    settings.json \
    application.conf

