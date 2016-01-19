#include <iostream>
#include <string>
using namespace std;

#include "lws.h"

int main(int argc, char *argv[])
{
    lws::Server server(3000);

    server.onConnection([](lws::Socket socket) {
        cout << "Connection" << endl;
        string str = "some message";
        socket.send(str, false);
    });

    server.onDisconnection([](lws::Socket socket) {
        cout << "Disconnection" << endl;
    });

    server.onMessage([](lws::Socket socket, std::string message) {
        cout << "Message: " << message << endl;
    });

    cout << "Running server on port 3000" << endl;
    server.run();
    return 0;
}
