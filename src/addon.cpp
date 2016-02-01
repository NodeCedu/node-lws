#include <node.h>
#include <node_object_wrap.h>
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
    static void run(const FunctionCallbackInfo<Value> &args);
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
    NODE_SET_PROTOTYPE_METHOD(tpl, "run", run);
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
    struct SocketABI {
        void *wsi;
        void *extension;
    };

    SocketABI *sa = (SocketABI *) &socket;

    // Share this template!
    Local<ObjectTemplate> t = ObjectTemplate::New(isolate);
    t->SetInternalFieldCount(2);

    Local<Object> s = t->NewInstance();
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

    if (!strcmp(*String::Utf8Value(args[0]->ToString()), "connection")) {
        connectionCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server.onConnection([isolate](lws::Socket socket) {
            *socket.getUser() = nullptr;
            HandleScope hs(isolate);
            Local<Value> argv[] = {wrapSocket(socket, isolate)};
            Local<Function>::New(isolate, connectionCallback)->Call(Null(isolate), 1, argv);
        });
    }
    else if (!strcmp(*String::Utf8Value(args[0]->ToString()), "close")) {
        closeCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server.onDisconnection([isolate](lws::Socket socket) {
            HandleScope hs(isolate);
            Local<Value> argv[] = {wrapSocket(socket, isolate)};
            Local<Function>::New(isolate, closeCallback)->Call(Null(isolate), 1, argv);
            delete ((string *) *socket.getUser());
        });
    }
    else if (!strcmp(*String::Utf8Value(args[0]->ToString()), "message")) {
        messageCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server.onMessage([isolate](lws::Socket socket, std::string message) {
            HandleScope hs(isolate);
            Local<Value> argv[] = {wrapSocket(socket, isolate), String::NewFromUtf8(isolate, message.c_str())};
            Local<Function>::New(isolate, messageCallback)->Call(Null(isolate), 2, argv);
        });
    }
}

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

void Server::run(const FunctionCallbackInfo<Value> &args)
{
    lws::Server *server = &ObjectWrap::Unwrap<Server>(args.Holder())->server;

    //async(launch::async, [server] {
        server->run();
    //});
}

void Server::send(const FunctionCallbackInfo<Value> &args)
{
    lws::Socket socket = unwrapSocket(args[0]->ToObject());
    string data = *String::Utf8Value(args[1]->ToString());
    socket.send(data, false);
}
