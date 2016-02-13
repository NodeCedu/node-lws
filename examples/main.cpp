/* this is a simple echo server */

#include <iostream>
#include <string>
using namespace std;

#include "lws.h"

int main(int argc, char *argv[])
{
    lws::Server server(3000);

    server.onConnection([](lws::Socket socket) {

    });

    server.onDisconnection([](lws::Socket socket) {

    });

    server.onMessage([](lws::Socket socket, char *data, size_t length, bool binary) {
        socket.send(data, length, binary);
    });

    cout << "Running echo server on port 3000" << endl;
    server.run();
    return 0;
}
