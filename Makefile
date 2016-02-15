default:
	for path in node_versions/*; do NODE=$$path make `(uname -s)`; done
	cp README.md dist/README.md
	cp LICENSE dist/LICENSE
Linux:
	g++ -std=c++11 -O3 -shared -I $$NODE/include -fPIC -DLIBUV_BACKEND src/addon.cpp src/lws.cpp -l:libwebsockets.a -s -o dist/lws_linux_`$$NODE/bin/node -e "console.log(process.versions.modules)"`.node
Darwin:
	g++ -std=c++11 -stdlib=libc++ -DLIBUV_BACKEND -mmacosx-version-min=10.7 -O3 -shared -fPIC src/addon.cpp src/lws.cpp -l:libwebsockets.a -s -o dist/lws_darwin.node
clean:
	rm -f dist/README.md
	rm -f dist/LICENSE
	rm -f dist/lws_*.node

