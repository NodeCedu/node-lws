#ifndef LWS_H
#define LWS_H

#include <string>
#include <vector>
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
    bool binary;
    char *buffer;
    unsigned int length;
};

struct Socket {
private:
    clws::lws *wsi;
    void *extension;
    std::vector<char *> messageQueue;
public:
    Socket(clws::lws *wsi, void *extension);
    void send(std::string &data, bool binary);
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
    Server(unsigned int port);
    void onConnection(std::function<void(Socket)> connectionCallback);
    void onMessage(std::function<void(Socket, std::string message)> messageCallback);
    void onDisconnection(std::function<void(Socket)> disconnectionCallback);
    void run();
};

}

#endif // LWS_H
