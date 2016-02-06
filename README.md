# Lightweight WebSockets for Node.js
```node-lws``` is a [libwebsockets](https://libwebsockets.org/index.html) wrapper for Node.js and C++. It implements an interface similar to the one available in [ws](https://github.com/websockets/ws).

By using the ridiculously lightweight ```libwebsockets``` as a foundation, ```node-lws``` *significantly* outperforms ```ws``` in both memory usage and cpu time. Since ```ws``` is self entitled "fastest" and "blazingly fast", ```node-lws``` can only be described as "fastester" and "blazinglier fast".

**NOTE:** This project started **Jan 13, 2016** and is at version **0.0.12**. Things are nearing a somewhat not-completely-broken status but will need some more time to stabilize. Please use the issue tracker to report feature requests and other opinions.

## Installing
```npm install lws``` is your friend. Node 4.x support (ABI 46). Linux ~~& Mac OS X 10.7+~~.

## Overview
```javascript
var lws = require('lws');
var server = new lws.Server(3000);

server.on('connection', function (socket) {
    console.log('[Connection]');
    server.send(socket, new Buffer('a text message'), false);
    server.send(socket, new Buffer('a binary message'), true);
    server.setUserData(socket, 'persistent per socket data');
});

server.on('message', function (socket, message, binary) {
    console.log('[Message: ' + message + ']');
    server.send(socket, new Buffer('You sent me this: \"' + message + '\"'), false);
});

server.on('close', function (socket) {
    console.log('[Close]');
    console.log(server.getUserData(socket));
});

console.log('Running server on port 3000');
```
### Class: lws.Server

#### new lws.Server(port)
Constructs a new Server object listening to ```port```.

#### Event: 'connection'
```javascript
function (socket) { }
```

Emitted when a connection has been established. ```socket``` is a *copy* of the internal socket structure. Changes made to this copy does not persist - use ```setUserData``` to set persistent data on a ```socket```.

#### Event: 'message'
```javascript
function (socket, message, binary) { }
```

Emitted when a message has been received. ```message``` is a Node.js Buffer. This buffer is *only valid inside this callback*. When this callback goes out of scope, the internal memory will be invalidated. You need to make a *deep copy* of the message if you want to keep it for later.

#### Event: 'close'
```javascript
function (socket) { }
```

Emitted when a connection has been closed. Data set with ```setUserData``` is valid until this callback goes out of scope.

#### setUserData(socket, data)

Used to set persistent private data on ```socket```. Currently the type is string, meaning you can store objects as JSON.

#### getUserData(socket)

Returns the private persistent data set on ```socket```.

### send(message, binary)

Queue a Node.js Buffer for sending. This function call makes exactly one internal memory allocation and one memory copy. ```message``` is sent as binary if the (boolean) ```binray``` flag is ```true```.

### ~~sendPaddedBuffer(paddedBuffer, binary)~~

Not yet implemented.

### ~~broadcast(message, binary)~~

Not yet implemented.

### ~~broadcastPaddedBuffer(paddedBuffer, binary)~~

Not yet implemented.

## Manual compilation
If the pre-compiled binaries don't suffice, you could compile the wrapper manually. It should compile for Node 0.12+. Start by cloning, configuring, compiling and installing libwebsockets:
```
git clone https://github.com/warmcat/libwebsockets.git
cd libwebsockets
git checkout libuv-test
cmake -DLWS_WITH_LIBUV=1 -DLWS_WITH_LIBEV=0 -DCMAKE_C_FLAGS=-fPIC -DLWS_WITHOUT_TESTAPPS=1 -DCMAKE_INSTALL_PREFIX=/usr .
make
sudo make install
```
node-lws can then be compiled in a pretty straight forward way.
