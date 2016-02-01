default:
	make `(uname -s)`
	cp README.md dist/README.md
	cp LICENSE dist/LICENSE
Linux:
	g++ -std=c++11 -O3 -shared -fPIC -DLWS_USE_LIBUV -I/usr/include/node src/addon.cpp src/lws.cpp -l:libwebsockets.a -luv -lssl -lcrypto -s -o dist/lws_linux.node
Darwin:
	g++ -std=c++11 -stdlib=libc++ -DLWS_USE_LIBUV -mmacosx-version-min=10.7 -O3 -shared -fPIC -I/usr/include/node src/addon.cpp src/lws.cpp -l:libwebsockets.a -luv -lssl -lcrypto -s -o dist/lws_darwin.node
clean:
	rm -f dist/README.md
	rm -f dist/LICENSE
	rm -f dist/lws_linux.node
	rm -f dist/lws_darwin.node
