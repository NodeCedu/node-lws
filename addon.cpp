#include <node.h>
#include <node_object_wrap.h>
using namespace v8;

#include "lws.h"

class Server : public node::ObjectWrap {
public:
    static void Init(v8::Local<v8::Object> exports);
    lws::Server server;

private:
    Server(unsigned int port);
    ~Server();

    static void New(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void on(const v8::FunctionCallbackInfo<v8::Value> &args);
    static void run(const FunctionCallbackInfo<Value> &args);
    static v8::Persistent<v8::Function> constructor;
};

void InitAll(Local<Object> exports)
{
    Server::Init(exports);
}

NODE_MODULE(lws, InitAll)

Persistent<Function> Server::constructor;

Server::Server(unsigned int port) : server(port)
{

}

Server::~Server()
{
}

void Server::Init(Local<Object> exports)
{
    Isolate *isolate = v8::Isolate::GetCurrent();

    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "Server"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    NODE_SET_PROTOTYPE_METHOD(tpl, "on", on);
    NODE_SET_PROTOTYPE_METHOD(tpl, "run", run);

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

#include <iostream>
using namespace std;

#include <cstring>

void Server::on(const FunctionCallbackInfo<Value> &args)
{
    lws::Server &server = ObjectWrap::Unwrap<Server>(args.Holder())->server;

    if (!strcmp(*v8::String::Utf8Value(args[0]->ToString()), "connection")) {
        cout << "Registering connection callback!" << endl;

        server.onConnection([](lws::Socket socket) {
            cout << "Connection" << endl;
        });
    }
    else if (!strcmp(*v8::String::Utf8Value(args[0]->ToString()), "close")) {
        cout << "Registering close callback!" << endl;

        server.onDisconnection([](lws::Socket socket) {
            cout << "Close" << endl;
        });
    }
    else if (!strcmp(*v8::String::Utf8Value(args[0]->ToString()), "message")) {
        cout << "Registering message callback!" << endl;

        server.onMessage([](lws::Socket socket, std::string message) {
            cout << "Message: " << message << endl;
        });
    }

    //

    //Server *obj = ObjectWrap::Unwrap<Server>(args.Holder());
    //obj->value_ += 1;

    //args.GetReturnValue().Set(Number::New(isolate, obj->value_));
}

void Server::run(const FunctionCallbackInfo<Value> &args)
{
    lws::Server &server = ObjectWrap::Unwrap<Server>(args.Holder())->server;

    cout << "Running server now" << endl;
    server.run();
}
