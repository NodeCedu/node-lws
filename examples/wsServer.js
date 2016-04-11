var lws = require('../dist/lws');
var wss = new lws.wsServer({ port: 3000 });

wss.on('connection', function (ws) {
	console.log('Connected');
	ws.on('message', function (message) {
		console.log('Received: ' + message);
	});

	ws.on('close', function () {
		console.log('Closed');
	});

	ws.send('Welcome!');
});
