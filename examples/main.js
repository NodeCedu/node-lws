var lws = require('lws');
var server = new lws.Server({ port: 3000 });

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
