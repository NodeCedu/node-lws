# Lightweight WebSockets for Node.js
```node-lws``` (or simply ```lws```) is a [libwebsockets](https://libwebsockets.org/index.html) wrapper for Node.js and C++. It exposes an easy to use interface much like the one available in [ws](https://github.com/websockets/ws). In contrast to ```ws```, ```node-lws``` is very lightweight and scales to millions of connections where ```ws``` simply chokes due to excessive memory usage.

* ```node-lws``` uses less than 40% the memory per connection compared to ```ws```.
* ```node-lws``` establishes connections in less than 10% the time compared to ```ws```.
* ```node-lws``` echoes messages in less than 30% the time compared to ```ws```.

[Primus](https://github.com/primus/primus#lws) support is included in 5.0. Use 'lws' as transformer & report any issues here.
The [ws interface](https://github.com/websockets/ws/blob/master/doc/ws.md) is exposed as `lws.wsServer` as opposed to ("core") `lws.Server`.

## Installation
[![](https://nodei.co/npm/lws.png)](https://www.npmjs.com/package/lws)

```
npm install --save lws
```

* Node 4.x, 5.x support (ABI 46-47)
* Linux & Mac OS X 10.7+

Please use the [issue tracker](https://github.com/alexhultman/node-lws/issues) to report bugs, feature requests and other opinions. If you have questions, don't hesitate to post.

## Overview
```javascript
var lws = require('lws');
var server = new lws.Server({ port: 3000 });

server.on('error', function (error) {
    console.log('Oops!');
});

server.on('upgrade', function (socket, headers) {
    console.log('Upgrading to WebSocket!');
    console.log(headers);
});

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

#### new lws.Server(options)
Constructs a new Server object. ```options``` is an object with these fields:

* port : Integer
* path : String
* keepAliveTime : Integer
* keepAliveInterval : Integer
* keepAliveRetry : Integer
* bufferSize : Integer
* perMessageDeflate : Boolean | Object
  * serverNoContextTakeover : Boolean
  * clientNoContextTakeover : Boolean
  * serverMaxWindowBits : Integer
  * clientMaxWindowBits : Integer
  * memLevel : Integer
* ssl : Object
  * key : String (path)
  * cert : String (path)
  * ca : String (path)
  * ciphers : String
  * rejectUnauthorized : Boolean

#### *A note on sockets*
An lws socket is just a pointer, so using it past 'close' is invalid (the memory it points to has been deleted). If you need data (objects, strings, etc) to survive past the 'close' event you need to create that resource in JS-land and attach it to the socket. Any function taking a socket will (potentially, most definitely) crash or hang the process, if that socket has been closed.

If you want a fail safe (yet heavier) interface you could check out `lws.wsServer` that aims to mimic `ws` in behavior as closely as possible.

#### Event: 'upgrade'
```javascript
function (socket, headers) { }
```
Emitted when a WebSocket upgrade request has been received. ```headers``` is an object with header names and corresponding values. Return ```true``` to filter this connection, closing it before handling the upgrade.

#### Event: 'connection'
```javascript
function (socket) { }
```

Emitted when a connection has been established. ```socket``` is a *copy* of the internal socket structure. Changes made to this copy does not persist - use ```setUserData``` to set persistent data on a ```socket```.

#### Event: 'error'
```javascript
function (error) { }
```
Emitted when a server error has been raised. ```error``` argument is reserved for future extension and is currently always ```undefined```.

#### Event: 'message'
```javascript
function (socket, message, binary, remainingBytes) { }
```

Emitted when a message has been received. ```message``` is a Node.js Buffer. This buffer is *only valid inside this callback*. When this callback goes out of scope, the internal memory will be invalidated. You need to make a *deep copy* of the message if you want to keep it for later.

Long messages will be split up into multiple fragments. ```remainingBytes``` will be zero once the message has been delivered in whole. In all other cases you need to buffer up the fragments or simply process them in a streaming fashion. Use server option ```bufferSize``` to set the maximum allowed fragment size.

#### Event: 'close'
```javascript
function (socket) { }
```

Emitted when a connection has been closed. Data set with ```setUserData``` is valid until this callback goes out of scope.

#### setUserData(socket, data)
```javascript
server.setUserData(socket, object);
```

Used to set persistent data on ```socket```. If you pass an object or string as data, only the reference will be copied.

#### getUserData(socket)
```javascript
var object = server.getUserData(socket);
```

Returns a reference to the object, string or value set on ```socket``` using ```setUserData```.

#### getFd(socket)
```javascript
var fd = server.getFd(socket);
```

Returns the underlying file descriptor of the socket. This can be used to identify ```socket``` and when storing the socket in a vector or map.

#### send(socket, message, binary)
```javascript
server.send(socket, buffer, false);
```

Queue a Node.js Buffer for sending. This function call makes at least one internal memory allocation and one memory copy. ```message``` is sent as binary if the (boolean) ```binary``` flag is ```true```.

#### close(socket)
```javascript
server.close(socket);
```
Gracefully close the ```socket``` by shutting it down. If any messages are peding they will be sent before closing the socket. This will not trigger the 'close' event.

#### prepareBuffer(buffer)
```javascript
var preparedBuffer = server.prepareBuffer(buffer);
```

Prepares a Node.js Buffer for zero-copy sending. The passed buffer is copied into a larger, padded buffer which can be passed to ```sendPrepared``` for increased sending performance. Suitable when sending the same message many times, e.g. when broadcasting.

The returned buffer *has to* be passed into at least one `sendPrepared` call to be properly deleted. Make sure not to call this function without at least one matching call to `sendPrepared`.

#### sendPrepared(socket, message, binary)
```javascript
server.sendPrepared(socket, buffer, false);
```

Same as ```send``` but for prepared buffers. See ```prepareBuffer``` for details.

#### handleUpgrade(socket, request, head)
```javascript
var webSocket = server.handleUpgrade(socket, request, head);
```
For use with the built-in ```http.Server```. Call this function from the ```http.Server 'upgrade'``` event to import and upgrade the connection. This function will return either ```undefined``` or the upgraded WebSocket. Events 'upgrade' and 'connection' will not be triggered by this call.

#### Event: 'http'
```javascript
function (socket, request) { }
```
Emitted when a regular HTTP request has been received. This event is not emitted for WebSocket upgrade requests.
