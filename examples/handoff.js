var http = require('http');
var lws = require('../dist/lws');

var httpServer = http.createServer(function (request, response) {
    response.writeHead(200, {'Content-Type': 'text/plain'});
    response.end('Well hello there!');
});

// note: instead of port you should specify server to get upgrade requests from
var server = new lws.Server({ port: 4000 });

httpServer.on('upgrade', function (request, socket, head) {
    // note: this will be less explicit

    // get fd and upgrade header
    var fd = socket._handle.fd;

    // fake the header (debug)
    var header = "GET /default HTTP/1.1\r\n" +
                "Host: server.example.com\r\n" +
                "Upgrade: websocket\r\n" +
                "Connection: Upgrade\r\n" +
                "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n" +
                "Sec-WebSocket-Protocol: default\r\n" +
                "Sec-WebSocket-Version: 13\r\n" +
                "Origin: http://example.com\r\n\r\n";

    // dup fd
    var newFd = server.dupSocket(fd);
    // destroy old fd and net.Socket
    socket.destroy();
    // adopt it
    server.adoptSocket(newFd, header);
});

// this will only trigger if the upgrade went through
server.on('connection', function (socket) {
    console.log('[Connection]');
    server.send(socket, new Buffer('a text message'), false);
    server.send(socket, new Buffer('a binary message'), true);
    server.setUserData(socket, 'persistent per socket data');
});

httpServer.listen(3000, '127.0.0.1', function () {

    var options = {
      port: 3000,
      hostname: '127.0.0.1',
      headers: {
        'Connection': 'Upgrade',
        'Upgrade': 'websocket'
      }
    };

    var req1 = http.request(options);
    req1.end();

    req1.on('upgrade', (res, socket, upgradeHead) => {
      console.log('got upgraded from node-lws!');


        /*var req2 = http.request(options);
        req2.end();

        req2.on('upgrade', (res, socket, upgradeHead) => {
            console.log('got upgraded from node-lws!');


            socket.end();
        });*/

      socket.end();
      process.exit(0);
    });
});
