var lws = require("./build/Release/lws");

var server = new lws.Server(3000);

server.on('connection', function (socket) {
	console.log('Connection');
});

server.on('message', function (socket, message) {
	console.log('Message: ' + message);
});

server.on('close', function (socket) {
	console.log('Disconnection');
});

// Shouldn't be needed later on!
server.run();
