var http = require('http');
var lws = require('../dist/lws');

var httpServer = http.createServer(function (request, response) {
    response.writeHead(200, {'Content-Type': 'text/plain'});
    response.end('Well hello there!');
});

// note: instead of port you should specify server to get upgrade requests from
var server = new lws.Server({ port: 4000 });
var connections = 0;

httpServer.on('upgrade', function (request, socket, head) {
    server.handleUpgrade(socket._handle.fd, request.headers['sec-websocket-key']);
    socket.destroy();
});

server.on('connection', function (socket) {
    connections++;
    if (connections % 1000 == 0) {
        console.log('Connections: ' + connections);
    }
});

server.on('message', function (socket, message, binary) {
    console.log('[Message: ' + message + ']');
    server.send(socket, new Buffer('You sent me this: \"' + message + '\"'), false);
});

server.on('close', function (socket) {
    connections--;
    if (connections % 1000 == 0) {
        console.log('Connections: ' + connections);
    }
});

httpServer.listen(3000);
