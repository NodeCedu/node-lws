#include "lws.h"

#include <node/node.h>
#include <node/node_buffer.h>
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

NODE_MODULE(lws, Main)

// these could be stored in the Server object as 3 aligned pointers?
Persistent<Function> connectionCallback, closeCallback, messageCallback;
Persistent<Object> persistentSocket;

void Main(Local<Object> exports)
{
    Isolate *isolate = exports->GetIsolate();
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, constructor);
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_SET_PROTOTYPE_METHOD(tpl, "on", on);
    NODE_SET_PROTOTYPE_METHOD(tpl, "send", send);
    NODE_SET_PROTOTYPE_METHOD(tpl, "setUserData", setUserData);
    NODE_SET_PROTOTYPE_METHOD(tpl, "getUserData", getUserData);

    exports->Set(String::NewFromUtf8(isolate, "Server"), tpl->GetFunction());

    Local<ObjectTemplate> socketTemplate = ObjectTemplate::New(isolate);
    socketTemplate->SetInternalFieldCount(2);
    persistentSocket.Reset(isolate, socketTemplate->NewInstance());
}

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

void constructor(const FunctionCallbackInfo<Value> &args)
{
    if (args.IsConstructCall()) {
        Local<Object> options = args[0]->ToObject();
        int port = options->Get(String::NewFromUtf8(args.GetIsolate(), "port"))->ToInteger()->Value();
        String::Utf8Value path(options->Get(String::NewFromUtf8(args.GetIsolate(), "path")));
        int keepAliveTime = options->Get(String::NewFromUtf8(args.GetIsolate(), "keepAliveTime"))->ToInteger()->Value();
        int keepAliveInterval = options->Get(String::NewFromUtf8(args.GetIsolate(), "keepAliveInterval"))->ToInteger()->Value();
        int keepAliveRetry = options->Get(String::NewFromUtf8(args.GetIsolate(), "keepAliveRetry"))->ToInteger()->Value();

        MaybeLocal<Value> perMessageDeflate = options->Get(String::NewFromUtf8(args.GetIsolate(), "perMessageDeflate"));
        bool usePerMessageDeflate = true;
        static string strPerMessageDeflate = "permessage-deflate";
        if (perMessageDeflate.ToLocalChecked()->IsObject()) {
            Local<Array> propertyNames = perMessageDeflate.ToLocalChecked()->ToObject()->GetPropertyNames();
            for (int i = 0; i < propertyNames->Length(); i++) {
                String::Utf8Value keyName(propertyNames->Get(i));
                MaybeLocal<Value> optionalValue = perMessageDeflate.ToLocalChecked()->ToObject()->Get(propertyNames->Get(i));
                if (!(optionalValue.ToLocalChecked()->IsBoolean() && !optionalValue.ToLocalChecked()->BooleanValue())) {
                    strPerMessageDeflate += "; " + camelCaseToUnderscore(string(*keyName, keyName.length()));

                    if (!optionalValue.IsEmpty() && !optionalValue.ToLocalChecked()->IsBoolean()) {
                        String::Utf8Value valueString(optionalValue.ToLocalChecked()->ToString());
                        strPerMessageDeflate += "=" + string(*valueString, valueString.length());
                    }
                }
            }
        }
        else if(perMessageDeflate.ToLocalChecked()->IsBoolean()) {
            usePerMessageDeflate = perMessageDeflate.ToLocalChecked()->BooleanValue();
        }

#ifdef VERBOSE_SERVER
        cout << "Using port = " << port << ", path = " << string(*path, path.length())
             << ", keepAliveTime = " << keepAliveTime << ", keepAliveInterval = "
             << keepAliveInterval << ", keepAliveRetry = " << keepAliveRetry << endl;

        if (usePerMessageDeflate) {
            cout << "Using perMessageDeflate:" << endl << strPerMessageDeflate << endl;
        }
#endif

        lws::Server *server;
        try {
            server = new lws::Server(port, string(*path, path.length()).c_str(), keepAliveTime, keepAliveRetry,
                                     keepAliveInterval, usePerMessageDeflate, strPerMessageDeflate.c_str());
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

void on(const FunctionCallbackInfo<Value> &args)
{
    lws::Server *server = (lws::Server *) args.Holder()->GetAlignedPointerFromInternalField(0);
    Isolate *isolate = args.GetIsolate();

    String::Utf8Value eventName(args[0]->ToString());
    if (!strncmp(*eventName, "error", eventName.length()) && !server) {
        Local<Function>::Cast(args[1])->Call(Null(isolate), 0, nullptr);
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
            delete ((string *) *socket.getUser());
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

// stores strings for now
void setUserData(const FunctionCallbackInfo<Value> &args)
{
    lws::Socket socket = unwrapSocket(args[0]->ToObject());
    *socket.getUser() = new string(*String::Utf8Value(args[1]->ToString()));
}

void getUserData(const FunctionCallbackInfo<Value> &args)
{
    lws::Socket socket = unwrapSocket(args[0]->ToObject());
    args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), ((string *) *socket.getUser())->c_str()));
}

//Server.send(Buffer, Boolean)
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
