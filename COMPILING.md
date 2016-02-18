# Compilation instructions
### libwebsockets
1. Clone & enter
  * ```git clone https://github.com/warmcat/libwebsockets.git```
  * ```cd libwebsockets```
2. Configure libwebsockets according to platform
  * **Linux:** ```cmake -DLWS_WITH_LIBUV=1 -DLWS_WITH_LIBEV=0 -DCMAKE_C_FLAGS=-fPIC -DLWS_WITHOUT_TESTAPPS=1 -DCMAKE_INSTALL_PREFIX=/usr .```
  * **OS X:** ```cmake -DLWS_WITH_LIBUV=1 -DLWS_WITH_LIBEV=0 -DCMAKE_C_FLAGS=-fPIC -DLWS_WITHOUT_TESTAPPS=1 -DMACOSX_DEPLOYMENT_TARGET=10.7 .```
  * Make sure configuration worked, install missing dependencies otherwise.
3. Compile and install
  * ```make```
  * ```sudo make install```
  
### node-lws
1. Clone
  * ```git clone https://github.com/alexhultman/node-lws.git```
2. Extract Node.js versions
  * Download latest 5.x and 4.x versions for your platform: https://nodejs.org/dist/
  * Extract packages into node-lws/node_versions (see README.md in this folder for examples)
3. Enter & compile node-lws
  * ```cd node-lws```
  * ```make```
  
You'll now have binaries in the node-lws/dist folder named: ```lws_<platform kernel name>_<ABI version>.node```
* One for every Node.js (ABI) version put in node-lws/node_versions.

### Detailed information
More detailed information can be inferred from looking at Makefile and lws.js.
