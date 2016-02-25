#include "lws.h"

#include <node.h>
#include <node_buffer.h>
#include <cstring>
#include <iostream>
#include <future>

using namespace std;
using namespace v8;

void Main(Local<Object> exports);
void constructor(const FunctionCallbackInfo<Value> &args);
void on(const FunctionCallbackInfo<Value> &args);
void send(const FunctionCallbackInfo<Value> &args);
void setUserData(const FunctionCallbackInfo<Value> &args);
void getUserData(const FunctionCallbackInfo<Value> &args);
void getFd(const FunctionCallbackInfo<Value> &args);
void adoptSocket(const FunctionCallbackInfo<Value> &args);

NODE_MODULE(lws, Main)

// these could be stored in the Server object as 3 aligned pointers?
Persistent<Function> connectionCallback, closeCallback, messageCallback, httpCallback, upgradeCallback;
Persistent<Object> persistentSocket, persistentHeaders;

void Main(Local<Object> exports)
{
    Isolate *isolate = exports->GetIsolate();
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, constructor);
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(tpl, "on", on);
    NODE_SET_PROTOTYPE_METHOD(tpl, "send", send);
    NODE_SET_PROTOTYPE_METHOD(tpl, "setUserData", setUserData);
    NODE_SET_PROTOTYPE_METHOD(tpl, "getUserData", getUserData);
    NODE_SET_PROTOTYPE_METHOD(tpl, "getFd", getFd);
    NODE_SET_PROTOTYPE_METHOD(tpl, "adoptSocket", adoptSocket);

    exports->Set(String::NewFromUtf8(isolate, "Server"), tpl->GetFunction());

    Local<ObjectTemplate> socketTemplate = ObjectTemplate::New(isolate);
    socketTemplate->SetInternalFieldCount(2);
    persistentSocket.Reset(isolate, socketTemplate->NewInstance());

    Local<ObjectTemplate> headersTemplate = ObjectTemplate::New(isolate);
    persistentHeaders.Reset(isolate, headersTemplate->NewInstance());
}

// helpers
string camelCaseToUnderscore(string camelCase)
{
    string underscore;
    for (char c : camelCase) {
        if (isupper(c)) {
            underscore += "_";
            underscore += tolower(c);
        }
        else {
            underscore += c;
        }
    }
    return underscore;
}

string getString(Isolate *isolate, Local<Object> &object, const char *key)
{
    Local<String> v8Key = String::NewFromUtf8(isolate, key);
    if (object->Has(v8Key)) {
        String::Utf8Value v8String(object->Get(v8Key));
        return string(*v8String, v8String.length());
    }
    return "";
}

int getInt(Isolate *isolate, Local<Object> &object, const char *key)
{
    Local<String> v8Key = String::NewFromUtf8(isolate, key);
    if (object->Has(v8Key)) {
        return object->Get(v8Key)->ToInteger()->Value();
    }
    return 0;
}

bool getObject(Isolate *isolate, Local<Object> &object, const char *key, Local<Object> &output)
{
    Local<String> v8Key = String::NewFromUtf8(isolate, key);
    if (object->Has(v8Key) && object->Get(v8Key)->IsObject()) {
        output = object->Get(v8Key)->ToObject();
        return true;
    }
    return false;
}

bool getBool(Isolate *isolate, Local<Object> &object, const char *key, bool &output)
{
    Local<String> v8Key = String::NewFromUtf8(isolate, key);
    if (object->Has(v8Key) && object->Get(v8Key)->IsBoolean()) {
        output = object->Get(v8Key)->BooleanValue();
    }
    return false;
}

const char *nullOrC(string &str)
{
    if (str.length()) {
        return str.c_str();
    }
    return nullptr;
}

void constructor(const FunctionCallbackInfo<Value> &args)
{
    if (args.IsConstructCall()) {
        Local<Object> options = args[0]->ToObject();
        int port = getInt(args.GetIsolate(), options, "port");
        string path = getString(args.GetIsolate(), options, "path");
        int keepAliveTime = getInt(args.GetIsolate(), options, "keepAliveTime");
        int keepAliveInterval = getInt(args.GetIsolate(), options, "keepAliveInterval");
        int keepAliveRetry = getInt(args.GetIsolate(), options, "keepAliveRetry");

#ifdef VERBOSE_SERVER
        cout << "Using port = " << port << ", path = " << path
             << ", keepAliveTime = " << keepAliveTime << ", keepAliveInterval = "
             << keepAliveInterval << ", keepAliveRetry = " << keepAliveRetry << endl;
#endif

        bool usePerMessageDeflate = true;
        static string strPerMessageDeflate = "permessage-deflate";
        Local<Object> perMessageDeflate;
        if (getObject(args.GetIsolate(), options, "perMessageDeflate", perMessageDeflate)) {
            Local<Array> propertyNames = perMessageDeflate->GetPropertyNames();
            for (int i = 0; i < propertyNames->Length(); i++) {
                String::Utf8Value propertyName(propertyNames->Get(i));
                Local<Value> propertyValue = perMessageDeflate->Get(propertyNames->Get(i));
                if (!(propertyValue->IsBoolean() && !propertyValue->BooleanValue())) {
                    strPerMessageDeflate += "; " + camelCaseToUnderscore(string(*propertyName, propertyName.length()));

                    if (!propertyValue->IsBoolean()) {
                        String::Utf8Value propertyValueString(propertyValue->ToString());
                        strPerMessageDeflate += "=" + string(*propertyValueString, propertyValueString.length());
                    }
                }
            }

#ifdef VERBOSE_SERVER
            cout << "Using perMessageDeflate:" << endl << strPerMessageDeflate << endl;
#endif
        }
        else {
            getBool(args.GetIsolate(), options, "perMessageDeflate", usePerMessageDeflate);

#ifdef VERBOSE_SERVER
            cout << "Passed perMessageDeflate was boolean: " << usePerMessageDeflate << endl;
#endif
        }

        static string certPath, keyPath, caPath, ciphers;
        static bool rejectUnauthorized = true;
        Local<Object> sslOptions;
        if (getObject(args.GetIsolate(), options, "ssl", sslOptions)) {
            certPath = getString(args.GetIsolate(), sslOptions, "cert");
            keyPath = getString(args.GetIsolate(), sslOptions, "key");
            caPath = getString(args.GetIsolate(), sslOptions, "ca");
            ciphers = getString(args.GetIsolate(), sslOptions, "ciphers");
            getBool(args.GetIsolate(), sslOptions, "rejectUnauthorized", rejectUnauthorized);

#ifdef VERBOSE_SERVER
            cout << "SSL options: cert: " << certPath << ", key: " << keyPath << ", ca: " << caPath << caPath
                 << ", ciphers: " << ciphers << ", rejectUnauthorized: " << rejectUnauthorized << endl;
#endif
        }

        lws::Server *server;
        try {
            server = new lws::Server(port, nullOrC(path), keepAliveTime, keepAliveRetry,
                                     keepAliveInterval, usePerMessageDeflate, strPerMessageDeflate.c_str(),
                                     nullOrC(certPath), nullOrC(keyPath), nullOrC(caPath), nullOrC(ciphers), rejectUnauthorized);
        }
        catch (...) {
            server = nullptr;
        }

        args.This()->SetAlignedPointerInInternalField(0, server);
        args.GetReturnValue().Set(args.This());
    }
}

inline Local<Object> wrapSocket(lws::Socket *socket, Isolate *isolate)
{
    struct SocketWrapper : lws::Socket {
        Local<Object> wrap(Local<Object> s)
        {
            s->SetAlignedPointerInInternalField(0, wsi);
            s->SetAlignedPointerInInternalField(1, extension);
            return s;
        }
    };

    return ((SocketWrapper *) socket)->wrap(Local<Object>::New(isolate, persistentSocket));
}

inline lws::Socket unwrapSocket(Local<Object> object)
{
    return lws::Socket((lws::clws::lws *) object->GetAlignedPointerFromInternalField(0),
                                          object->GetAlignedPointerFromInternalField(1));
}

Local<Object> generateHeaders(Isolate *isolate, lws::Socket &socket)
{
    // optimize this! (keep const string names persistent, etc)
    Local<Object> headersObject = Local<Object>::New(isolate, persistentHeaders);
    int i = 0;
    for (char *headerName, *header; headerName = socket.getHeaderName(i); i++) {
        if (header = socket.getHeader(i)) {
            headersObject->Set(String::NewFromUtf8(isolate, headerName, String::kNormalString, strlen(headerName) - 1), String::NewFromUtf8(isolate, header));
        }
        else {
            headersObject->Delete(String::NewFromUtf8(isolate, headerName, String::kNormalString, strlen(headerName) - 1));
        }
    }
    return headersObject;
}

void on(const FunctionCallbackInfo<Value> &args)
{
    lws::Server *server = (lws::Server *) args.Holder()->GetAlignedPointerFromInternalField(0);
    Isolate *isolate = args.GetIsolate();

    String::Utf8Value eventName(args[0]->ToString());
    if (!strncmp(*eventName, "error", eventName.length()) && !server) {
        Local<Function>::Cast(args[1])->Call(Null(isolate), 0, nullptr);
    }
    else if (server && !strncmp(*eventName, "http", eventName.length())) {
        httpCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server->onHttp([isolate](lws::Socket socket, char *data, size_t length) {
            HandleScope hs(isolate);
            Local<Value> argv[] = {wrapSocket(&socket, isolate),
                                   node::Buffer::New(isolate, data, length, [](char *data, void *hint) {}, nullptr).ToLocalChecked(),
                                   generateHeaders(isolate, socket)};
            Local<Function>::New(isolate, httpCallback)->Call(Null(isolate), 3, argv);
        });
    }
    else if (server && !strncmp(*eventName, "upgrade", eventName.length())) {
        upgradeCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server->onUpgrade([isolate](lws::Socket socket) {
            HandleScope hs(isolate);
            Local<Value> argv[] = {wrapSocket(&socket, isolate), generateHeaders(isolate, socket)};
            Local<Function>::New(isolate, upgradeCallback)->Call(Null(isolate), 2, argv);
        });
    }
    else if (server && !strncmp(*eventName, "connection", eventName.length())) {
        connectionCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server->onConnection([isolate](lws::Socket socket) {
            *socket.getUser() = nullptr;
            HandleScope hs(isolate);
            Local<Value> argv[] = {wrapSocket(&socket, isolate)->Clone()};
            Local<Function>::New(isolate, connectionCallback)->Call(Null(isolate), 1, argv);
        });
    }
    else if (server && !strncmp(*eventName, "close", eventName.length())) {
        closeCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server->onDisconnection([isolate](lws::Socket socket) {
            HandleScope hs(isolate);
            Local<Value> argv[] = {wrapSocket(&socket, isolate)};
            Local<Function>::New(isolate, closeCallback)->Call(Null(isolate), 1, argv);
            delete ((Persistent<Value> *) *socket.getUser());
        });
    }
    else if (server && !strncmp(*eventName, "message", eventName.length())) {
        messageCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server->onMessage([isolate](lws::Socket socket, char *data, size_t length, bool binary) {
            HandleScope hs(isolate);
            Local<Value> argv[] = {wrapSocket(&socket, isolate),
                                   /*ArrayBuffer::New(isolate, data, length)*/
                                   node::Buffer::New(isolate, data, length, [](char *data, void *hint) {}, nullptr).ToLocalChecked(),
                                   Boolean::New(isolate, binary)};
            Local<Function>::New(isolate, messageCallback)->Call(Null(isolate), 3, argv);
        });
    }
}

void setUserData(const FunctionCallbackInfo<Value> &args)
{
    lws::Socket socket = unwrapSocket(args[0]->ToObject());
    if (*socket.getUser()) {
        ((Persistent<Value> *) *socket.getUser())->Reset(args.GetIsolate(), args[1]);
    }
    else {
        *socket.getUser() = new Persistent<Value>(args.GetIsolate(), args[1]);
    }
}

void getUserData(const FunctionCallbackInfo<Value> &args)
{
    lws::Socket socket = unwrapSocket(args[0]->ToObject());
    if (*socket.getUser()) {
        args.GetReturnValue().Set(Local<Value>::New(args.GetIsolate(), *(Persistent<Value> *) *socket.getUser()));
    }
}

void getFd(const FunctionCallbackInfo<Value> &args)
{
    lws::Socket socket = unwrapSocket(args[0]->ToObject());
    args.GetReturnValue().Set(Integer::NewFromUnsigned(args.GetIsolate(), socket.getFd()));
}

void adoptSocket(const FunctionCallbackInfo<Value> &args)
{
    lws::Server *server = (lws::Server *) args.Holder()->GetAlignedPointerFromInternalField(0);
    int fd = args[0]->IntegerValue();
    String::Utf8Value upgradeHeader(args[1]->ToString());
    server->adoptSocket(fd, (new string(*upgradeHeader, upgradeHeader.length()))->c_str(), upgradeHeader.length());
}

void send(const FunctionCallbackInfo<Value> &args)
{
    unwrapSocket(args[0]->ToObject())
            .send(node::Buffer::Data(args[1])
            , node::Buffer::Length(args[1])
            , args[2]->BooleanValue());
}

// todo: zero-copy from JavaScript
/*void sendPaddedBuffer(const FunctionCallbackInfo<Value> &args)
{
    unwrapSocket(args[0]->ToObject())
            .send(node::Buffer::Data(args[1])
            ,node::Buffer::Length(args[1])
            , false);
}*/
