if (process.platform == 'linux') {
	module.exports = require('./lws_linux.node');
}
else {
	module.exports = require('./lws_mac.node');
}
