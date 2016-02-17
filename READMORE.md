### ~~sendPaddedBuffer(paddedBuffer, binary)~~

Not yet implemented.

### ~~broadcast(message, binary)~~

Not yet implemented.

### ~~broadcastPaddedBuffer(paddedBuffer, binary)~~

Not yet implemented.

## Manual compilation
If the pre-compiled binaries don't suffice, you could compile the wrapper manually. It should compile for Node 0.12+. Start by cloning, configuring, compiling and installing libwebsockets:
```
git clone https://github.com/warmcat/libwebsockets.git
cd libwebsockets
cmake -DLWS_WITH_LIBUV=1 -DLWS_WITH_LIBEV=0 -DCMAKE_C_FLAGS=-fPIC -DLWS_WITHOUT_TESTAPPS=1 -DCMAKE_INSTALL_PREFIX=/usr .
make
sudo make install
```
node-lws can then be compiled in a pretty straight forward way.
