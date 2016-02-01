TEMPLATE = lib
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += lws.cpp \
    addon.cpp

HEADERS += \
    lws.h

INCLUDEPATH += /usr/include/node

LIBS += -l:libwebsockets.a -luv -lssl -lcrypto -s

QMAKE_CXXFLAGS += -fPIC -DLWS_USE_LIBUV
