QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ./include \
                "C:/Program Files/OpenSSL-Win64/include"

DEFINES+=QT_NO_FOREACH

SOURCES += \
    ./proxyGUI/main.cpp \
    ./proxyGUI/mainwindow.cpp \
    proxyGUI/qtstreambuf.cpp \
    threadPool.cpp \
    tcpServer.cpp \
    clientConnectionManager.cpp \
    ./dispatcher/winSelectDispatcher.cpp \
    ./json/JsonParser.cpp \

HEADERS += \
    ./proxyGUI/mainwindow.h \
    proxyGUI/qtstreambuf.h
    ./dispatcher/dispatcher.h

FORMS += \
    ./proxyGUI/mainwindow.ui

LIBS +=  -L "C:/Program Files/OpenSSL-Win64/lib/mingw/x64" -lcrypto -lssl -lws2_32

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
