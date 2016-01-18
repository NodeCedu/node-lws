#include "lws.h"

#include <stddef.h>
#include <stdarg.h>
#include <cstring>

// We use epoll on Linux, kqueue on OS X
#ifdef __APPLE__
#define LWS_FD_BACKEND EVBACKEND_KQUEUE
#else
#define LWS_FD_BACKEND EVBACKEND_EPOLL
#endif

#include <iostream>
using namespace std;

namespace lws
{
#include <ev.h>

namespace clws
{
#include <libwebsockets.h>
}

int callback(clws::lws *wsi, clws::lws_callback_reasons reason, void *user, void *in, size_t len)
{
    lws::ServerInternals *serverInternals = (lws::ServerInternals *) lws_context_user(lws_get_context(wsi));
    SocketExtension *ext = (SocketExtension *) user;

    switch (reason) {
    case clws::LWS_CALLBACK_SERVER_WRITEABLE:
    {
        lws_write(wsi, (unsigned char *) ext->buffer + LWS_SEND_BUFFER_PRE_PADDING, ext->length, ext->binary ? clws::LWS_WRITE_BINARY : clws::LWS_WRITE_TEXT);
        delete [] ext->buffer;
        ext->buffer = nullptr;
        break;
    }

    case clws::LWS_CALLBACK_ESTABLISHED:
    {
        ext->buffer = nullptr;
        serverInternals->connectionCallback({wsi, user});
        break;
    }

    case clws::LWS_CALLBACK_CLOSED:
    {
        // Mark socket as closed and delete eventual buffer
        ext->length = -1;
        if (ext->buffer) {
            delete [] ext->buffer;
            ext->buffer = nullptr;
        }

        serverInternals->disconnectionCallback({wsi, user});

        break;
    }

    case clws::LWS_CALLBACK_RECEIVE:
    {
        serverInternals->messageCallback({wsi, user}, std::string((char *) in, len));
        break;
    }
    default:
        break;
    }
    return 0;
}

Socket::Socket(clws::lws *wsi, void *extension) : wsi(wsi), extension(extension)
{

}

void Socket::send(string &data, bool binary)
{
    SocketExtension *ext = (SocketExtension *) extension;
    // Skip sending if socket has been closed
    if (ext->length != -1) {
        ext->length = data.length();
        ext->buffer = new char[LWS_SEND_BUFFER_PRE_PADDING + ext->length + LWS_SEND_BUFFER_POST_PADDING];
        memcpy(ext->buffer + LWS_SEND_BUFFER_PRE_PADDING, data.c_str(), ext->length);

        ext->binary = binary;

        // Request notification when writing is allowed
        lws_callback_on_writable(wsi);
    }
}

Server::Server(unsigned int port)
{
    clws::lws_set_log_level(0, nullptr);

    clws::lws_protocols *protocols = new clws::lws_protocols[2];
    protocols[0] = {"default", callback, sizeof(SocketExtension)};
    protocols[1] = {nullptr, nullptr, 0};

    clws::lws_context_creation_info info = {};
    info.port = port;
    info.protocols = protocols;
    info.gid = info.uid = -1;
    info.user = &internals;
    info.options = clws::LWS_SERVER_OPTION_LIBEV;

    // TCP keep-alive
    info.ka_time = 30;
    info.ka_probes = 3;
    info.ka_interval = 5;

    if (!(context = clws::lws_create_context(&info))) {
        throw;
    }

    clws::lws_sigint_cfg(context, 0, nullptr);
    clws::lws_initloop(context, loop = ev_loop_new(LWS_FD_BACKEND));
}

void Server::onConnection(void (*connectionCallback)(lws::Socket socket))
{
    internals.connectionCallback = connectionCallback;
}

void Server::onMessage(void (*messageCallback)(lws::Socket, string))
{
    internals.messageCallback = messageCallback;
}

void Server::onDisconnection(void (*disconnectionCallback)(lws::Socket))
{
    internals.disconnectionCallback = disconnectionCallback;
}

void Server::run()
{
    while(true) {
        ev_run(loop, EVRUN_ONCE);
    }
}

}
