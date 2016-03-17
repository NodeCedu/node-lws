var lws = require('../dist/lws');
var server = new lws.Server({ port: 3000, bufferSize: 1024000, maxMessageSize: 1024 * 1024 * 100 });
var connections = 0;

server.on('error', function (error) {
    console.log('Could not start server!');
});

server.on('connection', function (socket) {
    connections++;
    if (connections % 1000 == 0 || connections < 1000) {
        console.log('Connections: ' + connections);
    }
});

// we can echo per message (buffered, slow)
server.on('message', function (socket, message, binary) {
    server.send(socket, message, binary);
});

// or per fragment (nonbuffered, fast)
server.on('fragment', function (socket, message, binary, bytesRemaining) {
    //server.sendFragment(socket, message, binary, bytesRemaining);
});

server.on('close', function (socket) {
    connections--;
    if (connections % 1000 == 0 || connections < 1000) {
        console.log('Connections: ' + connections);
    }
});

console.log('Running echo server on port 3000');
