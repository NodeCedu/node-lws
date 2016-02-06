#include <node.h>
#include <node_object_wrap.h>
#include <node_buffer.h>
using namespace v8;

#include <cstring>
#include <iostream>
#include <future>
using namespace std;

#include "lws.h"

class Server : public node::ObjectWrap {
public:
    static void Init(Local<Object> exports);
    lws::Server server;

private:
    Server(unsigned int port);
    ~Server();

    static void New(const FunctionCallbackInfo<Value> &args);
    static void on(const FunctionCallbackInfo<Value> &args);
    static void send(const FunctionCallbackInfo<Value> &args);
    static void setUserData(const FunctionCallbackInfo<Value> &args);
    static void getUserData(const FunctionCallbackInfo<Value> &args);
    static Persistent<Function> constructor;
};

NODE_MODULE(lws, Server::Init)

Persistent<Function> connectionCallback, closeCallback, messageCallback;
Persistent<Function> Server::constructor;

Server::Server(unsigned int port) : server(port)
{

}

Server::~Server()
{
}

void Server::Init(Local<Object> exports)
{
    Isolate *isolate = exports->GetIsolate();

    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "Server"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    NODE_SET_PROTOTYPE_METHOD(tpl, "on", on);
    NODE_SET_PROTOTYPE_METHOD(tpl, "send", send);
    NODE_SET_PROTOTYPE_METHOD(tpl, "setUserData", setUserData);
    NODE_SET_PROTOTYPE_METHOD(tpl, "getUserData", getUserData);

    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(String::NewFromUtf8(isolate, "Server"), tpl->GetFunction());
}

void Server::New(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = args.GetIsolate();

    if (args.IsConstructCall()) {
        // Invoked as constructor: `new MyObject(...)`
        int port = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();
        Server *obj = new Server(port);
        obj->Wrap(args.This());
        args.GetReturnValue().Set(args.This());
    } else {
        // Invoked as plain function `MyObject(...)`, turn into construct call.
        const int argc = 1;
        Local<Value> argv[argc] = {args[0]};
        Local<Function> cons = Local<Function>::New(isolate, constructor);
        args.GetReturnValue().Set(cons->NewInstance(argc, argv));
    }
}

inline Local<Object> wrapSocket(lws::Socket socket, Isolate *isolate)
{
    // Clean this up!
    struct SocketABI {
        void *wsi;
        void *extension;
    };
    SocketABI *sa = (SocketABI *) &socket;

    Local<ObjectTemplate> socketTemplate = ObjectTemplate::New(isolate);
    socketTemplate->SetInternalFieldCount(2);

    Local<Object> s = socketTemplate->NewInstance();
    s->SetAlignedPointerInInternalField(0, sa->wsi);
    s->SetAlignedPointerInInternalField(1, sa->extension);
    return s;
}

inline lws::Socket unwrapSocket(Local<Object> object)
{
    return lws::Socket((lws::clws::lws *) object->GetAlignedPointerFromInternalField(0),
                                          object->GetAlignedPointerFromInternalField(1));
}

void Server::on(const FunctionCallbackInfo<Value> &args)
{
    lws::Server &server = ObjectWrap::Unwrap<Server>(args.Holder())->server;
    Isolate *isolate = args.GetIsolate();

    String::Utf8Value eventName(args[0]->ToString());
    if (!strncmp(*eventName, "connection", eventName.length())) {
        connectionCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server.onConnection([isolate](lws::Socket socket) {
            *socket.getUser() = nullptr;
            HandleScope hs(isolate);
            Local<Value> argv[] = {wrapSocket(socket, isolate)};
            Local<Function>::New(isolate, connectionCallback)->Call(Null(isolate), 1, argv);
        });
    }
    else if (!strncmp(*eventName, "close", eventName.length())) {
        closeCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server.onDisconnection([isolate](lws::Socket socket) {
            HandleScope hs(isolate);
            Local<Value> argv[] = {wrapSocket(socket, isolate)};
            Local<Function>::New(isolate, closeCallback)->Call(Null(isolate), 1, argv);
            delete ((string *) *socket.getUser());
        });
    }
    else if (!strncmp(*eventName, "message", eventName.length())) {
        messageCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server.onMessage([isolate](lws::Socket socket, char *data, size_t length, bool binary) {
            HandleScope hs(isolate);
            Local<Value> argv[] = {wrapSocket(socket, isolate),
                                   node::Buffer::New(isolate, data, length).ToLocalChecked(),
                                   Boolean::New(isolate, binary)};
            Local<Function>::New(isolate, messageCallback)->Call(Null(isolate), 3, argv);
        });
    }
}

// these should not copy strings but rather keep the original node::Buffer
void Server::setUserData(const FunctionCallbackInfo<Value> &args)
{
    lws::Socket socket = unwrapSocket(args[0]->ToObject());
    *socket.getUser() = new string(*String::Utf8Value(args[1]->ToString()));
}

void Server::getUserData(const FunctionCallbackInfo<Value> &args)
{
    lws::Socket socket = unwrapSocket(args[0]->ToObject());
    args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), ((string *) *socket.getUser())->c_str()));
}

//Server.send(Buffer, Boolean)
void Server::send(const FunctionCallbackInfo<Value> &args)
{
    unwrapSocket(args[0]->ToObject())
            .send(node::Buffer::Data(args[1])
            ,node::Buffer::Length(args[1])
            , args[2]->BooleanValue());
}

// todo: zero-copy from JavaScript
/*void Server::sendPaddedBuffer(const FunctionCallbackInfo<Value> &args)
{
    unwrapSocket(args[0]->ToObject())
            .send(node::Buffer::Data(args[1])
            ,node::Buffer::Length(args[1])
            , false);
}*/
