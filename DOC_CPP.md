### C++ documentation

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
