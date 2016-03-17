#include "lws.h"

#include <node.h>
#include <node_buffer.h>
#include <cstring>
#include <iostream>
#include <future>

using namespace std;
using namespace v8;

// these could be stored in the Server object as 3 aligned pointers?
Persistent<Function> connectionCallback, closeCallback, messageCallback, fragmentCallback, httpCallback, upgradeCallback;
Persistent<Object> persistentSocket, persistentHeaders;

// helpers
string camelCaseToUnderscore(string camelCase)
{
    string underscore;
    for (const char *c = camelCase.c_str(); *c; underscore += tolower(*c++)) {
        if (isupper(*c)) {
            underscore += '_';
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
    return str.length() ? str.c_str() : nullptr;
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
        int bufferSize = getInt(args.GetIsolate(), options, "bufferSize");
        int maxMessageSize = getInt(args.GetIsolate(), options, "maxMessageSize");

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
            for (unsigned int i = 0; i < propertyNames->Length(); i++) {
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
                                     nullOrC(certPath), nullOrC(keyPath), nullOrC(caPath), nullOrC(ciphers),
                                     rejectUnauthorized, bufferSize, maxMessageSize);
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
            return s;
        }
    };

    return ((SocketWrapper *) socket)->wrap(Local<Object>::New(isolate, persistentSocket));
}

inline lws::Socket unwrapSocket(Local<Object> object)
{
    return lws::Socket((lws::clws::lws *) object->GetAlignedPointerFromInternalField(0));
}

Local<Object> generateHeaders(Isolate *isolate, lws::Socket &socket)
{
    // optimize this! (keep const string names persistent, etc)
    Local<Object> headersObject = Local<Object>::New(isolate, persistentHeaders);
    int i = 0;
    for (char *headerName, *header; (headerName = socket.getHeaderName(i)); i++) {
        if ((header = socket.getHeader(i))) {
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

            // move this out of here!
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
    else if (server && !strncmp(*eventName, "fragment", eventName.length())) {
        fragmentCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server->onFragment([isolate](lws::Socket socket, char *data, size_t length, bool binary, size_t remainingBytes) {
            HandleScope hs(isolate);
            Local<Value> argv[] = {wrapSocket(&socket, isolate),
                                   node::Buffer::New(isolate, data, length, [](char *data, void *hint) {}, nullptr).ToLocalChecked(),
                                   Boolean::New(isolate, binary),
                                   Integer::NewFromUnsigned(isolate, remainingBytes)};
            Local<Function>::New(isolate, fragmentCallback)->Call(Null(isolate), 4, argv);
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

void handleUpgrade(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = args.GetIsolate();
    Local<Object> socketObject = args[0]->ToObject();
    Local<String> v8HandleKey = String::NewFromUtf8(isolate, "_handle");
    Local<Object> socketHandleObject = socketObject->Get(v8HandleKey)->ToObject();
    Local<String> v8FdKey = String::NewFromUtf8(isolate, "fd");
    int fd = socketHandleObject->Get(v8FdKey)->IntegerValue();

    Local<String> v8HeadersKey = String::NewFromUtf8(isolate, "headers");
    Local<Object> headersObject = args[1]->ToObject()->Get(v8HeadersKey)->ToObject();
    Local<String> v8SecKey = String::NewFromUtf8(isolate, "sec-websocket-key");
    String::Utf8Value upgradeKey(headersObject->Get(v8SecKey));

    // placeholder, assumes version 13
    static char upgradeHeader[] = "Host: host\r\n"
                                  "Upgrade: websocket\r\n"
                                  "Connection: Upgrade\r\n"
                                  "Sec-WebSocket-Version: 13\r\n"
                                  "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                                  "\r\n";

    char *secKey = &upgradeHeader[sizeof(upgradeHeader) - 29];
    memcpy(secKey, *upgradeKey, 24);

    lws::Server *server = (lws::Server *) args.Holder()->GetAlignedPointerFromInternalField(0);
    lws::Socket socket = server->adoptSocket(fd, upgradeHeader, sizeof(upgradeHeader) - 1);

    Local<String> v8DestroyKey = String::NewFromUtf8(isolate, "destroy");
    Local<Function>::Cast(socketObject->Get(v8DestroyKey))->Call(socketObject, 0, nullptr);

    if (socket) {
        args.GetReturnValue().Set(wrapSocket(&socket, isolate)->Clone());
    }
}

void close(const FunctionCallbackInfo<Value> &args)
{
    if (!args.Length()) {
        // shut down the server
        lws::Server *server = (lws::Server *) args.Holder()->GetAlignedPointerFromInternalField(0);
        server->close();

        // detach the server from this object
        delete server;
        args.Holder()->SetAlignedPointerInInternalField(0, nullptr);
    }
    else {
        // shut down the socket
        unwrapSocket(args[0]->ToObject()).close();
    }
}

void send(const FunctionCallbackInfo<Value> &args)
{
    // send strings as utf-8
    /*if (args[1]->IsString()) {
        String::Utf8Value v8String(args[1]);

        unwrapSocket(args[0]->ToObject())
                .send(*v8String
                , v8String.length()
                , args[2]->BooleanValue());

        return;
    }*/

    unwrapSocket(args[0]->ToObject())
            .send(node::Buffer::Data(args[1])
            , node::Buffer::Length(args[1])
            , args[2]->BooleanValue());
}

// this guy needs to be transferred into sendPrepared
void prepareBuffer(const FunctionCallbackInfo<Value> &args)
{
    int length = node::Buffer::Length(args[0]);

    // PRE BUFFER POST REF_COUNT
    int paddedLength = lws::Server::getPrePadding() + length + lws::Server::getPostPadding() + sizeof(int);
    char *paddedBufferData = new char[paddedLength];
    memcpy(paddedBufferData + lws::Server::getPrePadding(), node::Buffer::Data(args[0]), length);
    int *refCount = (int *) (paddedBufferData + paddedLength - sizeof(int));
    *refCount = 0;

    args.GetReturnValue().Set(node::Buffer::New(args.GetIsolate(), paddedBufferData, paddedLength, [](char *data, void *hint) {
        // the GC doesn't own this buffer, do not delete it!
    }, nullptr).ToLocalChecked());
}

void sendPrepared(const FunctionCallbackInfo<Value> &args)
{
    char *paddedBuffer = node::Buffer::Data(args[1]);
    int paddedLength = node::Buffer::Length(args[1]);
    int *refCount = (int *) (paddedBuffer + paddedLength - sizeof(int));
    (*refCount)++; // by removing this one, it should start crashing again!

    unwrapSocket(args[0]->ToObject())
            .send(paddedBuffer
            , paddedLength - lws::Server::getPrePadding() - lws::Server::getPostPadding() - sizeof(int)
            , args[2]->BooleanValue()
            , refCount);
}

void sendFragment(const FunctionCallbackInfo<Value> &args)
{
    unwrapSocket(args[0]->ToObject())
            .sendFragment(node::Buffer::Data(args[1])
            , node::Buffer::Length(args[1])
            , args[2]->BooleanValue()
            , args[3]->IntegerValue());
}

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
    NODE_SET_PROTOTYPE_METHOD(tpl, "handleUpgrade", handleUpgrade);
    NODE_SET_PROTOTYPE_METHOD(tpl, "prepareBuffer", prepareBuffer);
    NODE_SET_PROTOTYPE_METHOD(tpl, "sendPrepared", sendPrepared);
    NODE_SET_PROTOTYPE_METHOD(tpl, "sendFragment", sendFragment);
    NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);

    exports->Set(String::NewFromUtf8(isolate, "Server"), tpl->GetFunction());

    Local<ObjectTemplate> socketTemplate = ObjectTemplate::New(isolate);
    socketTemplate->SetInternalFieldCount(1);
    persistentSocket.Reset(isolate, socketTemplate->NewInstance());

    Local<ObjectTemplate> headersTemplate = ObjectTemplate::New(isolate);
    persistentHeaders.Reset(isolate, headersTemplate->NewInstance());
}

NODE_MODULE(lws, Main)
