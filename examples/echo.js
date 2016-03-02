var lws = require('../dist/lws');
var server = new lws.Server({ port: 3000 });
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

server.on('message', function (socket, message, binary) {
    server.send(socket, message, false);
});

server.on('close', function (socket) {
    connections--;
    if (connections % 1000 == 0 || connections < 1000) {
        console.log('Connections: ' + connections);
    }
});

console.log('Running echo server on port 3000');
