#ifndef LWS_H
#define LWS_H

#include <string>

namespace lws
{

namespace clws
{
struct lws_context;
struct lws;
}

struct Socket {
    clws::lws *wsi;
    void *extension;
};

struct SocketExtension {
    char *buffer;
    unsigned int length;
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
