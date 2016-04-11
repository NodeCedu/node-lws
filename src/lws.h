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

enum SocketState {
    FRAGMENT_START,
    FRAGMENT_MID,
    CLOSING
};

extern int LWS_BINARY;
extern int LWS_TEXT;

struct SocketExtension {
    void *user;
    char state = FRAGMENT_START;
    std::string *receiveBuffer = nullptr;

    struct Message {
        int flags;
        char *buffer;
        size_t length;
        int *refCount;
    };

    std::queue<Message> messages;
};

struct Socket {
protected:
    clws::lws *wsi;
public:
    Socket(clws::lws *wsi);
    void send(char *data, size_t length, bool binary);
    void send(char *paddedBuffer, size_t length, int flags, int *refCount);
    void sendFragment(char *data, size_t length, bool binary, size_t remainingBytes);
    char *getHeader(int header);
    char *getHeaderName(int header);
    int getFd();
    void close();
    void **getUser();

    operator bool()
    {
        return wsi;
    }
};

class Server;

struct ServerInternals {
    Server *server;
    int adoptFd = 0;
    bool closingDown = false;
    int pendingMessages = 0;
    size_t bufferSize;
    size_t maxMessageSize;
    Socket adoptedSocket = Socket(nullptr);
    std::function<void(Socket)> upgradeCallback;
    std::function<void(Socket, char *data, size_t length)> httpCallback;
    std::function<void(Socket)> connectionCallback;
    std::function<void(Socket, char *data, size_t length, bool binary)> messageCallback;
    std::function<void(Socket, char *data, size_t length, bool binary, size_t remainingBytes)> fragmentCallback;
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
    void *protocolsPtr, *extensionsPtr;
public:
    Server(unsigned int port, const char *protocolName = nullptr, unsigned int ka_time = 0, unsigned int ka_probes = 0,
           unsigned int ka_interval = 0, bool perMessageDeflate = true, const char *perMessageDeflateOptions = nullptr,
           const char *certPath = nullptr, const char *keyPath = nullptr, const char *caPath = nullptr,
           const char *ciphers = nullptr, bool rejectUnauthorized = true, size_t bufferSize = 0, size_t maxMessageSize = 1024 * 1024 * 16);
    void onHttp(std::function<void(lws::Socket, char *, size_t)> httpCallback);
    void onUpgrade(std::function<void(lws::Socket)> upgradeCallback);
    void onConnection(std::function<void(Socket)> connectionCallback);
    void onMessage(std::function<void(Socket, char *, size_t, bool)> messageCallback);
    void onFragment(std::function<void(Socket, char *, size_t, bool, size_t)> fragmentCallback);
    void onDisconnection(std::function<void(Socket)> disconnectionCallback);
    Socket adoptSocket(size_t fd, const char *header, size_t length);
    void run();
    void close(bool force = false);
    static size_t getPrePadding();
    static size_t getPostPadding();
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
