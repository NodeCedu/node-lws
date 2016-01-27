# Lightweight WebSockets for Node.js
```node-lws``` is a [libwebsockets](https://libwebsockets.org/index.html) wrapper for Node.js and C++. It implements an interface similar to the one available in [ws](https://github.com/websockets/ws).

By using the ridiculously lightweight ```libwebsockets``` as a foundation, ```node-lws``` *significantly* outperforms ```ws``` in both memory usage and cpu time. Since ```ws``` is self entitled "fastest" and "blazingly fast", ```node-lws``` can only be described as "fastester" and "blazinglier fast".

**NOTE:** This project is still very young and has some missing major features. Keep calm and wait for things to fall into place.

## Overview
Consider main.js:
```javascript
var lws = require('lws');

var server = new lws.Server(3000);
var numConnections = 0;

server.on('connection', function (socket) {
	if (++numConnections % 50 == 0) {
		console.log('[Connection] Current number of connections: ' + numConnections);
	}
});

server.on('message', function (socket, message) {
	console.log('[Message] ' + message);
});

server.on('close', function (socket) {
	if (--numConnections % 50 == 0) {
		console.log('[Disconnection] Current number of connections: ' + numConnections);
	}
});

console.log('Running server on port 3000');
server.run();
```
## Installing
```npm install lws``` is your friend. This will install pre-compiled binaries for Node 4.x (ABI 46) on Linux & Mac OS X 10.7+. We depend on ```libev``` which can be installed through your package manager (e.g. [Homebrew](http://brew.sh/) for OS X). Look for packages named libev or libev4.

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
todo
