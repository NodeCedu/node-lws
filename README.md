# lws: a (lightweight) node.js websocket library 
```lws``` is a libwebsockets wrapper for Node.js and C++. It implements a similar interface as the one available in ```ws```.
By using the C library libwebsockets as a base, it has been shown to scale 5-16x better in memory usage compared to ```ws```.

## Overview
todo
## Installing
```npm install lws``` will install lws.node, a binary C++ addon with dependencies on libc, libev, libuv, etc.

## Details
* libwebsockets implements v13 websocket protocol and is written by Andy Green. License is LGPL 2.1 with static linking exceptions.
* Connections require very minimal amounts of memory - 500k connections have been shown to require less than 3gb of memory, something requiring around 12gb of memory using ```ws```.
* libev is used as the event-loop, hooking up with Node.js's libuv to provide an epoll-based socket selection.

## Documentation
todo
