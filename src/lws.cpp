#include "lws.h"

#include <stddef.h>
#include <stdarg.h>
#include <cstring>

#ifndef LWS_USE_LIBUV
// We use epoll on Linux, kqueue on OS X
#ifdef __APPLE__
#define LWS_FD_BACKEND EVBACKEND_KQUEUE
#else
#define LWS_FD_BACKEND EVBACKEND_EPOLL
#endif
#endif

#include <iostream>
#include <new>
using namespace std;

namespace lws
{
#ifdef LWS_USE_LIBUV
#include <uv.h>
#else
#include <ev.h>
#endif

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
        SocketExtension::Message &message = ext->messages.front();
        lws_write(wsi, (unsigned char *) message.buffer + LWS_SEND_BUFFER_PRE_PADDING, message.length, message.binary ? clws::LWS_WRITE_BINARY : clws::LWS_WRITE_TEXT);
        if (message.owned) {
            delete [] message.buffer;
        }
        ext->messages.pop();
        if (!ext->messages.empty()) {
            lws_callback_on_writable(wsi);
        }
        break;
    }

    case clws::LWS_CALLBACK_ESTABLISHED:
    {
        new (&ext->messages) queue<SocketExtension::Message>;
        serverInternals->connectionCallback({wsi, ext});
        break;
    }

    case clws::LWS_CALLBACK_CLOSED:
    {
        while (!ext->messages.empty()) {
            delete [] ext->messages.front().buffer;
            ext->messages.pop();
        }
        ext->messages.~queue<SocketExtension::Message>();

        serverInternals->disconnectionCallback({wsi, ext});
        break;
    }

    case clws::LWS_CALLBACK_RECEIVE:
    {
        serverInternals->messageCallback({wsi, ext}, std::string((char *) in, len));
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
    char *paddedBuffer = new char[LWS_SEND_BUFFER_PRE_PADDING + data.length() + LWS_SEND_BUFFER_POST_PADDING];
    memcpy(paddedBuffer + LWS_SEND_BUFFER_PRE_PADDING, data.c_str(), data.length());
    send(paddedBuffer, data.length(), binary, true);
}

void Socket::send(char *paddedBuffer, size_t length, bool binary, bool transferOwnership)
{
    SocketExtension *ext = (SocketExtension *) extension;
    SocketExtension::Message message = {
        binary,
        paddedBuffer,
        length,
        transferOwnership
    };
    ext->messages.push(message);

    // Request notification when writing is allowed
    lws_callback_on_writable(wsi);
}

size_t Socket::getPrePadding()
{
    return LWS_SEND_BUFFER_PRE_PADDING;
}

size_t Socket::getPostPadding()
{
    return LWS_SEND_BUFFER_POST_PADDING;
}

Server::Server(unsigned int port, unsigned int ka_time, unsigned int ka_probes, unsigned int ka_interval)
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
    info.options = clws::LWS_SERVER_OPTION_LIBUV;
    info.ka_time = ka_time;
    info.ka_probes = ka_probes;
    info.ka_interval = ka_interval;

    if (!(context = clws::lws_create_context(&info))) {
        throw;
    }

#ifdef LWS_USE_LIBUV
    clws::lws_uv_sigint_cfg(context, 0, nullptr);
    clws::lws_uv_initloop(context, (lws::uv_loop_t *) (loop = uv_default_loop()), 0);
#else
    clws::lws_ev_initloop(context, loop = ev_loop_new(LWS_FD_BACKEND), 0);
    clws::lws_ev_sigint_cfg(context, 0, nullptr);
#endif
}

void Server::onConnection(function<void(lws::Socket)> connectionCallback)
{
    internals.connectionCallback = connectionCallback;
}

void Server::onMessage(function<void(lws::Socket, string)> messageCallback)
{
    internals.messageCallback = messageCallback;
}

void Server::onDisconnection(function<void(lws::Socket)> disconnectionCallback)
{
    internals.disconnectionCallback = disconnectionCallback;
}

void Server::run()
{
#ifdef LWS_USE_LIBUV
    while(true) {
        uv_run((uv_loop_t *) loop, UV_RUN_ONCE);
    }
#else
    while(true) {
        ev_run(loop, EVRUN_ONCE);
    }
#endif
}

}
