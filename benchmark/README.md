## Benchmark
This folder keeps the benchmark used to compare ```node-lws``` with other WebSocket server implementations.

By default, compiling the program using ```make``` will create a binary measuring:

* Establishment performance of 200k connections.
* Echo performance of 400 serial text messages.
* Memory consumption per connection (you have to read it yourself).

Modifying the macros, you can get this benchmark to establish 1 million connections with very little RAM usage. It connects to localhost using many addresses which you have to set up yourself (along with required system configurations).

Output looks like this for ```node-lws```:
```
----------------------------------
Active connections: 400
Idle connections: 199600
Connection performance: 23.5821 connections/ms
----------------------------------
Echo delay, microseconds: 22411
ALL DONE
```
and like this for ```ws```:
```
----------------------------------
Active connections: 400
Idle connections: 199600
Connection performance: 4.16467 connections/ms
----------------------------------
Echo delay, microseconds: 91055
ALL DONE
```
