var http = require('http');
var lws = require('../dist/lws');

var httpServer = http.createServer(function (request, response) {
    response.writeHead(200, {'Content-Type': 'text/plain'});
    response.end('Well hello there!');
});

var server = new lws.Server({});
var connections = 0;
var preparedBuffer = server.prepareBuffer(new Buffer('Welcome!'));

httpServer.on('upgrade', function (request, socket, head) {
    server.handleUpgrade(socket, request);
});

server.on('connection', function (socket) {
    connections++;
    if (connections % 1000 == 0) {
        console.log('Connections: ' + connections);
    }

    server.sendPrepared(socket, preparedBuffer, false);
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
