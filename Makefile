CPP_FILES := $(wildcard src/*.cpp)
LWS := ../libwebsockets/lib
C_FILES := $(LWS)/base64-decode.c $(LWS)/handshake.c $(LWS)/libwebsockets.c $(LWS)/service.c $(LWS)/pollfd.c $(LWS)/output.c $(LWS)/parsers.c $(LWS)/context.c $(LWS)/alloc.c $(LWS)/header.c $(LWS)/client.c $(LWS)/client-handshake.c $(LWS)/client-parser.c $(LWS)/ssl.c $(LWS)/sha-1.c $(LWS)/lws-plat-unix.c $(LWS)/server.c $(LWS)/server-handshake.c $(LWS)/extension.c $(LWS)/extension-permessage-deflate.c $(LWS)/libuv.c
C_FLAGS := -c -O3 -fPIC $(C_FILES) -I ../libwebsockets
CPP_FLAGS := -std=c++11 -O3 -I $(LWS) -I ../libwebsockets -shared -fPIC -DLIBUV_BACKEND $(CPP_FILES)

default:
	for path in node_versions/*; do if [ -d $$path ]; then NODE=$$path make `(uname -s)`; fi; done
	cp README.md dist/README.md
	cp LICENSE dist/LICENSE
Linux:
	gcc $(C_FLAGS) -I $$NODE/include/node
	g++ $(CPP_FLAGS) -I $$NODE/include/node `(ls *.o | tr '\n' ' ')` -s -o dist/lws_linux_`$$NODE/bin/node -e "console.log(process.versions.modules)"`.node
	rm -f *.o
Darwin:
	gcc -mmacosx-version-min=10.7 $(C_FLAGS) -I $$NODE/include/node
	g++ -stdlib=libc++ -mmacosx-version-min=10.7 -undefined dynamic_lookup $(CPP_FLAGS) -I $$NODE/include/node `(ls *.o | tr '\n' ' ')` -o dist/lws_darwin_`$$NODE/bin/node -e "console.log(process.versions.modules)"`.node
	rm -f *.o
clean:
	rm -f dist/README.md
	rm -f dist/LICENSE
	rm -f dist/lws_*.node
