//
// Load lws binary for the current system
// Shall be prebuilt for each platform i.e. lws.node
//

var currentPlatformDir = '/../deps/' + process.platform + '/' + process.arch;
var lws = require(currentPlatformDir + '/lws.node');

module.exports = lws;