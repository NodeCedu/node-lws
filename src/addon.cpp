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
    static Persistent<Function> constructor;
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
    Isolate *isolate = Isolate::GetCurrent();

    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "Server"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    NODE_SET_PROTOTYPE_METHOD(tpl, "on", on);
    NODE_SET_PROTOTYPE_METHOD(tpl, "run", run);
    NODE_SET_PROTOTYPE_METHOD(tpl, "send", send);

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

Local<Object> wrapSocket(lws::Socket socket)
{
    // ROFL!
    struct SocketABI {
        void *wsi;
        void *extension;
    };

    SocketABI *sa = (SocketABI *) &socket;

    // Share this template!
    Local<ObjectTemplate> t = ObjectTemplate::New(isolate);
    t->SetInternalFieldCount(2);

    // Wrap socket
    Local<Object> s = t->NewInstance();
    s->SetAlignedPointerInInternalField(0, sa->wsi);
    s->SetAlignedPointerInInternalField(1, sa->extension);
    return s;
}

void Server::on(const FunctionCallbackInfo<Value> &args)
{
    lws::Server &server = ObjectWrap::Unwrap<Server>(args.Holder())->server;

    isolate = args.GetIsolate();

    if (!strcmp(*String::Utf8Value(args[0]->ToString()), "connection")) {

        connectionCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server.onConnection([](lws::Socket socket) {

            const unsigned argc = 1;
            Local<Value> argv[argc] = {wrapSocket(socket)};
            Local<Function> f = Local<Function>::New(isolate, connectionCallback);
            f->Call(Null(isolate), argc, argv);

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
    else if (!strcmp(*String::Utf8Value(args[0]->ToString()), "close")) {

        closeCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server.onDisconnection([](lws::Socket socket) {
            const unsigned argc = 1;
            Local<Value> argv[argc] = {wrapSocket(socket)};
            Local<Function> f = Local<Function>::New(isolate, closeCallback);
            f->Call(Null(isolate), argc, argv);
        });
    }
    else if (!strcmp(*String::Utf8Value(args[0]->ToString()), "message")) {

        messageCallback.Reset(isolate, Local<Function>::Cast(args[1]));
        server.onMessage([](lws::Socket socket, std::string message) {
            const unsigned argc = 2;
            Local<Value> argv[argc] = {wrapSocket(socket), String::NewFromUtf8(isolate, message.c_str())};
            Local<Function> f = Local<Function>::New(isolate, messageCallback);
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

void Server::send(const FunctionCallbackInfo<Value> &args)
{
    Local<Object> s = args[0]->ToObject();
    lws::Socket socket((lws::clws::lws *) s->GetAlignedPointerFromInternalField(0),
                       s->GetAlignedPointerFromInternalField(1));

    string data = *String::Utf8Value(args[1]->ToString());
    socket.send(data, false);
}
