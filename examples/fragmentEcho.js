// an example of a very performant echo server

var lws = require('../dist/lws');
var server = new lws.Server({ port: 3000, bufferSize: 1024000 });
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

server.on('message', function (socket, message, binary, remainingBytes) {
	server.sendFragment(socket, message, binary, remainingBytes);
});

server.on('close', function (socket) {
    connections--;
    if (connections % 1000 == 0 || connections < 1000) {
        console.log('Connections: ' + connections);
    }
});

console.log('Running fragmented echo server on port 3000');
