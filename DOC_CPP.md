## C++ documentation

##### Overview
```c++
lws::Server server({
    port(3000),
    bufferSize(1024 * 16)
});

server.onConnection([](lws::Socket socket) {
    socket.send("Well hello there!");
});

server.onMessage([](lws::Socket socket, char *data, size_t length, bool binary) {
    cout << "Got a message: " << string((const char *) data, length) << endl;
});

server.onDisconnection([](lws::Socket socket) {
    cout << "Someone left us :(" << endl;
});

```

```c++
Server::Server(...)
```
Constructs an lws::Server. Options are messed up right now.

```c++
void onUpgrade(std::function<void(lws::Socket)> upgradeCallback);
```

```c++
void onConnection(std::function<void(Socket)> connectionCallback);
```
```c++
void onMessage(std::function<void(Socket, char *, size_t, bool)> messageCallback);
```
```c++
void onFragment(std::function<void(Socket, char *, size_t, bool, size_t)> fragmentCallback);
```
```c++
void onDisconnection(std::function<void(Socket)> disconnectionCallback);
```
