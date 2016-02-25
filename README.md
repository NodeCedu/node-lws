# Lightweight WebSockets for Node.js
```node-lws``` (or simply ```lws```) is a [libwebsockets](https://libwebsockets.org/index.html) wrapper for Node.js and C++. It exposes an easy to use interface much like the one available in [ws](https://github.com/websockets/ws). In contrast to ```ws```, ```node-lws``` is very lightweight and scales to millions of connections where ```ws``` simply chokes due to excessive memory usage.

* ```node-lws``` uses less than 40% the memory per connection compared to ```ws```.
* ```node-lws``` establishes connections in less than 10% the time compared to ```ws```.
* ```node-lws``` echoes messages in less than 30% the time compared to ```ws```.

## Installation
[![](https://nodei.co/npm/lws.png)](https://www.npmjs.com/package/lws)

```
npm install --save lws
```

* Node 4.x, 5.x support (ABI 46-47).
* Linux & Mac OS X 10.7+

NOTE: This project started Jan 13, 2016. Please use the issue tracker to report bugs, feature requests and other opinions. Also, there might be short periods of time where the (npm) published version is behind the version on [GitHub](https://github.com/alexhultman/node-lws).

## Overview
```javascript
var lws = require('lws');
var server = new lws.Server({ port: 3000 });

server.on('error', function (error) {
    console.log('Oops!');
});

server.on('http', function (socket, request) {
    console.log('Got some HTTP action: ' + request);
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

#### Event: 'http'
```javascript
function (socket, request) { }
```
Emitted when a regular HTTP request has been received. This event is not emitted for WebSocket upgrade requests.

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
function (socket, message, binary) { }
```

Emitted when a message has been received. ```message``` is a Node.js Buffer. This buffer is *only valid inside this callback*. When this callback goes out of scope, the internal memory will be invalidated. You need to make a *deep copy* of the message if you want to keep it for later.

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
