var lws = require('lws');
var server = new lws.Server(3000);

server.on('connection', function (socket) {
    console.log('[Connection]');
    server.send(socket, 'some message');
    server.send(socket, 'some other message');
    server.send(socket, 'some third message');
});

server.on('message', function (socket, message) {
    console.log('[Message: ' + message + ']');
    server.send(socket, 'two can playeth that game!');
});

server.on('close', function (socket) {
    console.log('[Close]');
});

console.log('Running server on port 3000');
server.run();
