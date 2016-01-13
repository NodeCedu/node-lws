all:
	make wrapper
	make addon
wrapper:
	g++ -O2 -std=c++11 main.cpp lws.cpp -lwebsockets -lev
addon:
	node-gyp configure build
