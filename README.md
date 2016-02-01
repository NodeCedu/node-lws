# Lightweight WebSockets for Node.js
```node-lws``` is a [libwebsockets](https://libwebsockets.org/index.html) wrapper for Node.js and C++. It implements an interface similar to the one available in [ws](https://github.com/websockets/ws).

By using the ridiculously lightweight ```libwebsockets``` as a foundation, ```node-lws``` *significantly* outperforms ```ws``` in both memory usage and cpu time. Since ```ws``` is self entitled "fastest" and "blazingly fast", ```node-lws``` can only be described as "fastester" and "blazinglier fast".

**NOTE:** This project is still very young and has some missing major features. Keep calm and wait for things to fall into place.

**ALSO NOTE:** Builds are not up-to-date and things can be completely broken at times. I'm working on it.

## Overview
Consider main.js:
```javascript
var lws = require('lws');
var server = new lws.Server(3000);

server.on('connection', function (socket) {
    console.log('[Connection]');
    server.send(socket, 'some message');
    server.send(socket, 'some other message');
    server.send(socket, 'some third message');
    server.setUserData(socket, 'this is private');
});

server.on('message', function (socket, message) {
    console.log('[Message: ' + message + ']');
    server.send(socket, 'two can playeth that game!');
});

server.on('close', function (socket) {
    console.log('[Close]');
    console.log(server.getUserData(socket));
});

console.log('Running server on port 3000');
```

## Installing
```npm install lws``` is your friend.
* Node 4.x support (ABI 46)
* Linux & Mac OS X 10.7+

### Manual compilation
If the pre-compiled binaries don't suffice, you could compile the wrapper manually. It should compile for Node 0.12+. Start by cloning, configuring, compiling and installing libwebsockets:
```
git clone https://github.com/warmcat/libwebsockets.git
cd libwebsockets
cmake -DLWS_WITH_LIBEV=1 -DCMAKE_C_FLAGS=-fPIC -DCMAKE_INSTALL_PREFIX:PATH=/usr .
make
sudo make install
```
node-lws can then be compiled in a pretty straight forward way.
## Documentation
The interface is expanding as we go (and will be re-designed in the future). Every functionality is currently implemented in the Server object:

* Server(port) *constructs and returns a Server object*
* Server.send(socket, "some string") *enqueues a string for sending* (needs work to reduce copying, Buffer support etc)
* Server.on('connect', function(socket)) *registers an event handler for connections*
* Server.on('message', function(socket, message)) *registers an event  handler for messages*
* Server.on('close', function(socket)) *registers an event handler for disconnections*
* Server.setUserData(socket, value) *sets the user data for this socket*
* Server.getUserData(socket) *returns the set user data for this socket*
* Server.broadcast("some shared string") *enqueues a string for sending on all open connections* [not implemented yet]
