/* this is a simple echo server */

#include <iostream>
#include <string>
using namespace std;

#include "lws.h"

int main(int argc, char *argv[])
{
    try {
        lws::Server server(3000);

        server.onConnection([](lws::Socket socket) {

        });

        server.onDisconnection([](lws::Socket socket) {

        });

        server.onMessage([](lws::Socket socket, char *data, size_t length, bool binary, size_t remainingBytes) {
            socket.sendFragment(data, length, binary, remainingBytes);
        });

        cout << "Running echo server on port 3000" << endl;
        server.run();
    }
    catch (...) {
        cout << "Could not start server!" << endl;
        return -1;
    }
    return 0;
}
