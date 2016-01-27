#ifndef LWS_H
#define LWS_H

#include <string>
#include <vector>

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
    void (*connectionCallback)(Socket socket);
    void (*messageCallback)(Socket socket, std::string message);
    void (*disconnectionCallback)(Socket socket);
};

class Server
{
private:
    struct ev_loop *loop;
    clws::lws_context *context;
    ServerInternals internals;
public:
    Server(unsigned int port);
    void onConnection(void (*connectionCallback)(Socket socket));
    void onMessage(void (*messageCallback)(Socket socket, std::string message));
    void onDisconnection(void (*disconnectionCallback)(Socket socket));
    void run();
};

}

#endif // LWS_H
