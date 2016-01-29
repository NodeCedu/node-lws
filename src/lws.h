#ifndef LWS_H
#define LWS_H

#include <string>
#include <queue>
#include <functional>

namespace lws
{

namespace clws
{
struct lws_context;
struct lws;
}

struct SocketExtension {
    void *user;

    struct Message {
        bool binary;
        char *buffer;
        size_t length;
        bool owned;
    };

    std::queue<Message> messages;
};

struct Socket {
private:
    clws::lws *wsi;
    void *extension;
public:
    Socket(clws::lws *wsi, void *extension);
    void send(std::string &data, bool binary);
    void send(char *paddedBuffer, size_t length, bool binary, bool transferOwnership);
    size_t getPrePadding();
    size_t getPostPadding();
    void **getUser()
    {
        return &((SocketExtension *) extension)->user;
    }
};

class Server;

struct ServerInternals {
    Server *server;
    std::function<void(Socket)> connectionCallback;
    std::function<void(Socket, std::string message)> messageCallback;
    std::function<void(Socket)> disconnectionCallback;
};

class Server
{
private:
    struct ev_loop *loop;
    clws::lws_context *context;
    ServerInternals internals;
public:
    Server(unsigned int port, unsigned int ka_time = 0, unsigned int ka_probes = 0, unsigned int ka_interval = 0);
    void onConnection(std::function<void(Socket)> connectionCallback);
    void onMessage(std::function<void(Socket, std::string message)> messageCallback);
    void onDisconnection(std::function<void(Socket)> disconnectionCallback);
    void run();
    struct ev_loop *getEventLoop()
    {
        return loop;
    }
};

}

#endif // LWS_H
