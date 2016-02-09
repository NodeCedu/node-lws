TEMPLATE = lib
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += lws.cpp \
    addon.cpp

HEADERS += \
    lws.h

LIBS += -l:libwebsockets.a -luv -lssl -lcrypto -s

QMAKE_CXXFLAGS += -fPIC -DLIBUV_BACKEND
