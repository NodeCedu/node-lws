TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += ../examples/main.cpp \
    lws.cpp

HEADERS += \
    lws.h

LIBS += -l:libwebsockets.a -luv -lssl -lcrypto

QMAKE_CXXFLAGS += -DLIBUV_BACKEND
