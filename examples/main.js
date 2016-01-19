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
