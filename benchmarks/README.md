Running ```make && ./bench1 500000 3000``` you will get results similar to these (500k connections):

Connection performance: 3.77718 connections/ms  
Memory performance: 51.9015 connections/mb

Connection performance: 67.1501 connections/ms  
Memory performance: 147.411 connections/mb

That is,

* ```node-lws``` uses less than 40% the memory per connection compared to ```ws```.
* ```node-lws``` establishes connections in less than 10% the time compared to ```ws```.

Running ```make && ./bench2 10000 3000``` you will get results similar to these (10k connections):

Echo performance: 25.5599 echoes/ms

Echo performance: 119.457 echoes/ms

That is,

* ```node-lws``` echoes messages in less than 30% the time compared to ```ws```.

Beyond these tests, we have also benchmarked the echoing of a large buffer a 100MB. In this test we found node-lws to achieve ~1GB/sec throughput while the ws server (with binary addons) achieved ~330MB/sec. Multiple runs were averaged.

---

For C++ developers, a similar comparison with WebSocket++ has been made:

Connection performance: 14.1675 connections/ms
Memory performance: 43.2579 connections/mb

* ```node-lws``` uses less than 30% the memory per connection compared to ```WebSocket++```.
* ```node-lws``` establishes connections in less than 25% the time compared to ```WebSocket++```.

Echo performance is curretly slightly better in WebSocket++:

Echo performance: 157.587 echoes/ms (WebSocket++)

vs.

Echo performance: 145.817 echoes/ms (node-lws run as a C++ server)

* ```node-lws``` echoes messages in less than 110% the time compared to ```WebSocket++```.
