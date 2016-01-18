# node-lws: a (lightweight) node.js websocket library
```node-lws``` is a [libwebsockets](https://libwebsockets.org/index.html) wrapper for Node.js and C++. It implements an interface similar to the one available in [ws](https://github.com/websockets/ws).

By using the ridiculously lightweight ```libwebsockets``` as a foundation, ```node-lws``` *significantly* outperforms ```ws``` in both memory usage and cpu time.

## Overview
Consider main.js:
```javascript
var lws = require("./build/Release/lws");

var server = new lws.Server(3000);
var numConnections = 0;

server.on('connection', function (socket) {
	if (++numConnections % 50 == 0) {
		console.log("[Connection] Current number of connections: " + numConnections);
	}
});

server.on('message', function (socket, message) {
	console.log('Message: ' + message);
});

server.on('close', function (socket) {
	if (--numConnections % 50 == 0) {
		console.log("[Disconnection] Current number of connections: " + numConnections);
	}
});

console.log("Running server on port 3000");
server.run();
```
## Installing
```npm install lws``` will install the pre-compiled binaries lws.node and libwebsockets.so.6 along with corresponding LGPL 2.1 licenses. Binaries are available for Mac and Linux systems and depend on libev, libssl and libcrypto.

## Details
* libwebsockets implements v13 websocket protocol and is written by Andy Green. License is LGPL 2.1 with static linking exceptions.
* Connections require very minimal amounts of memory - 500k connections have been shown to require less than 3gb of memory, something requiring around 12gb of memory using ```ws```.
* libev is used as the event-loop, hooking up with Node.js's libuv to provide an epoll-based socket selection.

## Documentation
todo
