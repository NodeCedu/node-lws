addon:
	cd src && node-gyp configure build
	cp README.md dist/README.md
	cp LICENSE dist/LICENSE
	cp src/build/Release/lws.node dist/lws_`uname -s | tr '[:upper:]' '[:lower:]')`.node
	rm -rf src/build
clean:
	rm -f dist/LICENSE
	rm -rf src/build
	rm -f dist/lws_linux.node
	rm -f dist/lws_darwin.node
	rm -f dist/README.md
