# lws: a (lightweight) node.js websocket library 
```lws``` is a libwebsockets wrapper for Node.js and C++. It implements a similar interface as the one available in ```ws```.
By using the C library libwebsockets as a base, it has been shown to scale 5-16x better in memory usage compared to ```ws```.

## Overview
lws currently supports acting as a WebSocket server using the following interface:
```javascript
var lws = require('lws'); // Load the native module
var server = new lws.Server({ port: 3000 }); // Create a server listening to port 3000

server.on('connection', function(socket) { // Register a callback for handling established connections
    socket.on('message', function(str) { // Register a callback for handling received messages
        console.log('message: ' + str);
    });
    socket.send('something sent on connection'); // Queue a string for transfer
});

```
The corresponding C++ interface which is being wrapped is:
```c++
#include <lws>

auto server = new lws::Server(3000);

server.onConnection([](Socket &socket) {
    socket.onMessage([](std::string &str) {
        cout << "message: " << str << endl;
    });
    socket.send("something sent on connection");
});

server.run();

```
## Installing
```npm install lws``` will install lws.node, a binary C++ addon with dependencies on libc, libev, libuv, etc.

## Details
* libwebsockets implements v13 websocket protocol and is written by Andy Green. License is LGPL 2.1 with static linking exceptions.
* Connections require very minimal amounts of memory - 500k connections have been shown to require less than 3gb of memory, something requiring around 12gb of memory using ```ws```.
* libev is used as the event-loop, hooking up with Node.js's libuv to provide an epoll-based socket selection.

## Documentation
todo
