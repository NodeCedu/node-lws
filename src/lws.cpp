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
#ifdef LIBUV_BACKEND
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
        do {
            // we can be called even though we have no messages ready!
            if (ext->messages.empty())
                break;

            SocketExtension::Message &message = ext->messages.front();
            lws_write(wsi, (unsigned char *) message.buffer + LWS_SEND_BUFFER_PRE_PADDING, message.length, message.binary ? clws::LWS_WRITE_BINARY : clws::LWS_WRITE_TEXT);
            if (message.owned) {
                delete [] message.buffer;
            }
            ext->messages.pop();
        } while(!ext->messages.empty() && !lws_partial_buffered(wsi));

        if (!ext->messages.empty()) {
            lws_callback_on_writable(wsi);
        }
        else {
            // we are now empty and ready to close the socket
            if (ext->closed) {
                shutdown(clws::lws_get_socket_fd(wsi), SHUT_RDWR);
            }
        }
        break;
    }

    /*case clws::LWS_CALLBACK_WSI_CREATE:
    {

        break;
    }

    case clws::LWS_CALLBACK_WSI_DESTROY:
    {

        break;
    }*/

    case clws::LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
    {
        *lws::Socket(wsi).getUser() = nullptr;
        if (serverInternals->upgradeCallback && !serverInternals->adoptFd) {
            serverInternals->upgradeCallback({wsi});
        }
        break;
    }

    case clws::LWS_CALLBACK_FILTER_HTTP_CONNECTION:
    {
        // currently we only support getting the headers and closing it
        // wip
        if (serverInternals->httpCallback) {
            serverInternals->httpCallback({wsi}, (char *) in, len);
        }
        //returning 1 here will close it!
        return 1;
        break;
    }

    case clws::LWS_CALLBACK_CLOSED_HTTP:
    {
        //cout << "Closed http connection!" << endl;
        break;
    }

    case clws::LWS_CALLBACK_ESTABLISHED:
    {
        ext->closed = false;
        new (&ext->messages) queue<SocketExtension::Message>;
        if (serverInternals->adoptFd == clws::lws_get_socket_fd(wsi)) {
            serverInternals->adoptedSocket = lws::Socket(wsi);
            break;
        }
        if (serverInternals->connectionCallback && !serverInternals->adoptFd) {
            serverInternals->connectionCallback({wsi});
        }
        break;
    }

    case clws::LWS_CALLBACK_CLOSED:
    {
        while (!ext->messages.empty()) {
            delete [] ext->messages.front().buffer;
            ext->messages.pop();
        }
        ext->messages.~queue<SocketExtension::Message>();

        // ignore close events that we triggered ourselves
        if (serverInternals->disconnectionCallback && !ext->closed) {
            serverInternals->disconnectionCallback({wsi});
        }
        break;
    }

    case clws::LWS_CALLBACK_RECEIVE:
    {
        if (serverInternals->messageCallback) {
            serverInternals->messageCallback({wsi}, (char *) in, len, lws_frame_is_binary(wsi));
        }
        break;
    }
    default:
        break;
    }
    return 0;
}

Socket::Socket(clws::lws *wsi) : wsi(wsi)
{

}

void Socket::send(char *data, size_t length, bool binary)
{
    // ignore sendings on socket in closing state
    SocketExtension *ext = (SocketExtension *) clws::lws_wsi_user(wsi);
    if (ext->closed) {
        return;
    }

    char *paddedBuffer = new char[LWS_SEND_BUFFER_PRE_PADDING + length + LWS_SEND_BUFFER_POST_PADDING];
    memcpy(paddedBuffer + LWS_SEND_BUFFER_PRE_PADDING, data, length);
    send(paddedBuffer, length, binary, true);
}

void Socket::send(char *paddedBuffer, size_t length, bool binary, bool transferOwnership)
{
    // todo: we should also ignore sending preparedBuffers

    SocketExtension *ext = (SocketExtension *) clws::lws_wsi_user(wsi);
    SocketExtension::Message message = {
        binary,
        paddedBuffer,
        length,
        transferOwnership
    };
    ext->messages.push(message);

    if (ext->messages.size() == 1) {
        lws_callback_on_writable(wsi);
    }
}

char *Socket::getHeader(int header)
{
    static char buf[1024];
    if (clws::lws_hdr_copy(wsi, buf, sizeof(buf), (clws::lws_token_indexes) header) < 1) {
        return nullptr;
    }
    return buf;
}

char *Socket::getHeaderName(int header)
{
    if (header > clws::WSI_TOKEN_COUNT) {
        return nullptr;
    }
    return (char *) clws::lws_token_to_string((clws::lws_token_indexes) header);
}

size_t Server::getPrePadding()
{
    return LWS_SEND_BUFFER_PRE_PADDING;
}

size_t Server::getPostPadding()
{
    return LWS_SEND_BUFFER_POST_PADDING;
}

int Socket::getFd()
{
    return clws::lws_get_socket_fd(wsi);
}

void Socket::close()
{
    SocketExtension *ext = (SocketExtension *) clws::lws_wsi_user(wsi);

    ext->closed = true;

    if (ext->messages.empty()) {
        shutdown(getFd(), SHUT_RDWR);
    }
}

void **Socket::getUser()
{
    return &((SocketExtension *) clws::lws_wsi_user(wsi))->user;
}

Server::Server(unsigned int port, const char *protocolName, unsigned int ka_time, unsigned int ka_probes, unsigned int ka_interval, bool perMessageDeflate,
               const char *perMessageDeflateOptions, const char *certPath, const char *keyPath, const char *caPath, const char *ciphers, bool rejectUnauthorized)
{
    clws::lws_set_log_level(0, nullptr);

    clws::lws_protocols *protocols = new clws::lws_protocols[2];
    protocols[0] = {protocolName ? protocolName : "default", callback, sizeof(SocketExtension)};
    protocols[1] = {nullptr, nullptr, 0};

    clws::lws_extension *extensions = new clws::lws_extension[2];
    memset(extensions, 0, sizeof(clws::lws_extension) * 2);
    if (perMessageDeflate) {
        extensions[0] = {"permessage-deflate", clws::lws_extension_callback_pm_deflate, perMessageDeflateOptions ? perMessageDeflateOptions : "permessage-deflate"};
        extensions[1] = {nullptr, nullptr, 0};
    }

    clws::lws_context_creation_info info = {};
    info.port = port;

    info.ssl_cert_filepath = certPath;
    info.ssl_private_key_filepath = keyPath;
    info.ssl_ca_filepath = caPath;
    info.ssl_cipher_list = ciphers;
    if (!rejectUnauthorized) {
        info.options |= clws::LWS_SERVER_OPTION_ALLOW_NON_SSL_ON_SSL_PORT;
    }

    info.protocols = protocols;
    info.extensions = extensions;

    info.gid = info.uid = -1;
    info.user = &internals;
#ifdef LIBUV_BACKEND
    info.options |= clws::LWS_SERVER_OPTION_LIBUV;
#else
    info.options |= clws::LWS_SERVER_OPTION_LIBEV;
#endif
    info.ka_time = ka_time;
    info.ka_probes = ka_probes;
    info.ka_interval = ka_interval;

    if (!(context = clws::lws_create_context(&info))) {
        delete [] protocols;
        delete [] extensions;
        throw nullptr;
    }

#ifdef LIBUV_BACKEND
    clws::lws_uv_sigint_cfg(context, 0, nullptr);

    // this is a train wreck right now
    clws::lws_uv_initloop(context, (lws::uv_loop_t *) (loop = uv_default_loop()),[](uv_signal_t *handle, int signum) {
            exit(0);
    }, 0);
#else
    clws::lws_ev_initloop(context, loop = ev_loop_new(LWS_FD_BACKEND), 0);
    clws::lws_ev_sigint_cfg(context, 0, nullptr);
#endif
}

void Server::onHttp(function<void(lws::Socket, char *, size_t)> httpCallback)
{
    internals.httpCallback = httpCallback;
}

void Server::onUpgrade(function<void(lws::Socket)> upgradeCallback)
{
    internals.upgradeCallback = upgradeCallback;
}

void Server::onConnection(function<void(lws::Socket)> connectionCallback)
{
    internals.connectionCallback = connectionCallback;
}

void Server::onMessage(function<void(lws::Socket, char *, size_t, bool)> messageCallback)
{
    internals.messageCallback = messageCallback;
}

void Server::onDisconnection(function<void(lws::Socket)> disconnectionCallback)
{
    internals.disconnectionCallback = disconnectionCallback;
}

Socket Server::adoptSocket(size_t fd, const char *header, size_t length)
{
#ifndef __APPLE__
#define dup clws::dup
#endif

    internals.adoptFd = dup(fd);
    internals.adoptedSocket = Socket(nullptr);
    clws::lws_adopt_socket_readbuf(context, internals.adoptFd, header, length);
    internals.adoptFd = 0;
    return internals.adoptedSocket;
}

void Server::run()
{
#ifdef LIBUV_BACKEND
    uv_run((uv_loop_t *) loop, UV_RUN_DEFAULT);
#else
    while(true) {
        ev_run(loop, EVRUN_ONCE);
    }
#endif
}

void Server::close()
{
    // ignore handling this currently, fix later on!
    return;

    // remove the close listener, we do not want to tigger it
    internals.disconnectionCallback = [](lws::Socket s){

    };

    // we get hangs and valgrind hell from free(context)
    // we cannot use context in any way from the point
    // it has been freed!
    clws::lws_context_destroy(context);
}

}
