#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <chrono>
#include <errno.h>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <vector>
using namespace std;

typedef websocketpp::client<websocketpp::config::asio_client> client;
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
typedef client::connection_ptr connection_ptr;

#define CONNECTIONS_PER_ADDRESS 28000
#define TOTAL_CONNECTIONS 200000//1000000
#define IDLE_CONNECTIONS_PER_ACTIVE_CONNECTION 499
#define PORT 3000
#define ECHO_MESSAGE "This will be echoed"

client clientIO;
bool connected, echoed;
int connections = 0;
int address = 1;
vector<websocketpp::connection_hdl> heavyConnections;
chrono::time_point<chrono::system_clock> startPoint;

void updateAddress()
{
    if (++connections % 1000 == 0) {
        cout << "Connections: " << connections << endl;
    }

    if (connections % CONNECTIONS_PER_ADDRESS == 0) {
        address++;
    }
}

int nextLightweightConnection()
{
    int socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // this most likely means we cannot open more file descriptors
    if (socketfd == -1) {
        cout << "Socket invalid: " << errno << endl;
        cout << "Connections: " << connections << endl;
        return -2;
    }

    // connect to 127.0.0.1 - 127.0.0.X
    // where X is around CONNECTIONS / CONNECTIONS_PER_ADDRESS
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(("127.0.0." + to_string(address)).c_str());
    addr.sin_port = htons(PORT);

    char *buf = "GET /default HTTP/1.1\r\n"
                "Host: server.example.com\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"
                "Sec-WebSocket-Protocol: default\r\n"
                "Sec-WebSocket-Version: 13\r\n"
                "Origin: http://example.com\r\n\r\n";

    char message[1024];

    int err = connect(socketfd, (sockaddr *) &addr, sizeof(addr));
    if (err) {
        cout << "Some connection error: " << err << endl;
        cout << "Connections: " << connections << endl;
        return -1;
    }
    send(socketfd, buf, strlen(buf), 0);
    memset(message, 0, 1024);
    size_t length;
    do {
        length = recv(socketfd, message, sizeof(message), 0);
    }
    while (strncmp(&message[length - 4], "\r\n\r\n", 4));

    updateAddress();
    return 0;
}

void nextHeavyweightConnection()
{
    connected = false;
    string uri = "ws://127.0.0." + to_string(address) + ":" + to_string(PORT);
    websocketpp::lib::error_code ec;
    connection_ptr con = clientIO.get_connection(uri, ec);
    clientIO.connect(con);

    while(!connected) {
        clientIO.run_one();
    }

    updateAddress();
}

void echoMessage(int i)
{
    echoed = false;

    clientIO.send(heavyConnections[i], ECHO_MESSAGE, websocketpp::frame::opcode::value::TEXT);

    while(!echoed) {
        clientIO.run_one();
    }
}

int main(int argc, char **argv)
{
    clientIO.clear_access_channels(websocketpp::log::alevel::all);
    clientIO.init_asio();

    clientIO.set_message_handler([](websocketpp::connection_hdl hdl, client::message_ptr msg) {
        if (ECHO_MESSAGE != msg->get_payload()) {
            cout << "Error echo" << endl;
            exit(-1);
        }
        echoed = true;
    });

    clientIO.set_open_handler([&clientIO](websocketpp::connection_hdl hdl) {
        connected = true;
        heavyConnections.push_back(hdl);
    });

    startPoint = chrono::system_clock::now();
    while(connections < TOTAL_CONNECTIONS) {

        // Idle connections are made very lightweight
        for (int i = 0; i < IDLE_CONNECTIONS_PER_ACTIVE_CONNECTION
             && connections < TOTAL_CONNECTIONS; i++) {
            if (nextLightweightConnection()) {
                return -1;
            }
        }

        // Active connections are "real" using WebSocket++
        nextHeavyweightConnection();
    }

    cout << "----------------------------------" << endl;

    cout << "Active connections: " << heavyConnections.size() << endl;
    cout << "Idle connections: " << (connections - heavyConnections.size()) << endl;
    float connectionsPerSecond = float(connections) / chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - startPoint).count();
    cout << "Connection performance: " << connectionsPerSecond << " connections/ms" << endl;

    // start sending messages using the active connections
    cout << "----------------------------------" << endl;
    startPoint = chrono::system_clock::now();
    for (int i = 0; i < heavyConnections.size(); i++) {
        echoMessage(i);
    }
    cout << "Echo delay, microseconds: " << chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - startPoint).count() << endl;

    cout << "ALL DONE" << endl;
    clientIO.run();

    return 0;
}
