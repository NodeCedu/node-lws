addon:
	node-gyp configure build
	cp README.md ./npm/README.md
linux:
	make addon
	cp ./build/Release/lws.node ./npm/lws_linux.node
	cp /usr/lib/libwebsockets.so.6 ./npm/libwebsockets.so.6
mac:
	make addon
	cp ./build/Release/lws.node ./npm/lws_mac.node
