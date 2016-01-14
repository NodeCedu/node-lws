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

NODE_MODULE(lws, Server::Init)

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

// These should be captured
Persistent<Function> connectionCallback, closeCallback, messageCallback;
Isolate *isolate;

#include <uv.h>

auto *uvLoop = uv_default_loop();

void Server::on(const FunctionCallbackInfo<Value> &args)
{
    lws::Server &server = ObjectWrap::Unwrap<Server>(args.Holder())->server;

    isolate = args.GetIsolate();

    if (!strcmp(*v8::String::Utf8Value(args[0]->ToString()), "connection")) {

        connectionCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server.onConnection([](lws::Socket socket) {

            Local<Function> f = Local<Function>::New(isolate, connectionCallback);
            f->Call(Null(isolate), 0, nullptr);

            // Fore libuv to call the callbacks from main thread
            /*uv_queue_work(uvLoop, nullptr, [](uv_work_s *user){
                cout << "From thread" << endl;
            }, [](uv_work_s *user, int status) {
                cout << "From main thread" << endl;
                Local<Function> f = Local<Function>::New(isolate, connectionCallback);
                f->Call(Null(isolate), 0, nullptr);
            });*/
        });
    }
    else if (!strcmp(*v8::String::Utf8Value(args[0]->ToString()), "close")) {

        closeCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server.onDisconnection([](lws::Socket socket) {

            Local<Function> f = Local<Function>::New(isolate, closeCallback);
            f->Call(Null(isolate), 0, nullptr);
        });
    }
    else if (!strcmp(*v8::String::Utf8Value(args[0]->ToString()), "message")) {

        messageCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server.onMessage([](lws::Socket socket, std::string message) {

            Local<Function> f = Local<Function>::New(isolate, messageCallback);

            const unsigned argc = 2;
            Local<Value> argv[argc] = { String::NewFromUtf8(isolate, "socket object here"), String::NewFromUtf8(isolate, message.c_str()) };
            f->Call(Null(isolate), argc, argv);

        });
    }
}

void Server::run(const FunctionCallbackInfo<Value> &args)
{
    lws::Server *server = &ObjectWrap::Unwrap<Server>(args.Holder())->server;

    //async(launch::async, [server] {
        server->run();
    //});
}
