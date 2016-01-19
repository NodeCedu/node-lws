all:
	make wrapper
	make addon
wrapper:
	g++ -O2 -std=c++11 ./src/main.cpp ./src/lws.cpp -lwebsockets -lev
addon:
	node-gyp configure build
