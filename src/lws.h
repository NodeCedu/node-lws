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
protected:
    clws::lws *wsi;
    void *extension;
public:
    Socket(clws::lws *wsi, void *extension);
    void send(char *data, size_t length, bool binary);
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
    std::function<void(Socket, char *data, size_t length, bool binary)> messageCallback;
    std::function<void(Socket)> disconnectionCallback;
};

class Server
{
private:
#ifdef LIBUV_BACKEND
    void *loop;
#else
    struct ev_loop *loop;
#endif
    clws::lws_context *context;
    ServerInternals internals;
public:
    Server(unsigned int port, const char *protocolName = nullptr, unsigned int ka_time = 0, unsigned int ka_probes = 0,
           unsigned int ka_interval = 0, bool perMessageDeflate = true, const char *perMessageDeflateOptions = nullptr,
           const char *certPath = nullptr, const char *keyPath = nullptr, const char *caPath = nullptr,
           const char *ciphers = nullptr, bool rejectUnauthorized = true);
    void onConnection(std::function<void(Socket)> connectionCallback);
    void onMessage(std::function<void(Socket, char *, size_t, bool)> messageCallback);
    void onDisconnection(std::function<void(Socket)> disconnectionCallback);
    void run();
#ifdef LIBUV_BACKEND
    void *getEventLoop()
    {
        return loop;
    }
#else
    struct ev_loop *getEventLoop()
    {
        return loop;
    }
#endif
};

}

#endif // LWS_H
