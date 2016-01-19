addon:
	node-gyp configure build
	cp README.md ./npm/README.md
linux:
	make addon
	cp ./build/Release/lws.node ./npm/lws_linux.node
darwin:
	make addon
	cp ./build/Release/lws.node ./npm/lws_darwin.node
