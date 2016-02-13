Running ```make && ./bench1 500000 3000``` you will get results similar to these (500k connections):

Connection performance: 3.77718 connections/ms  
Memory performance: 51.9015 connections/mb

Connection performance: 50.1555 connections/ms  
Memory performance: 133.728 connections/mb

That is,

* ```node-lws``` uses less than 40% the memory per connection compared to ```ws```.
* ```node-lws``` establishes connections in less than 10% the time compared to ```ws```.

Running ```make && ./bench2 10000 3000``` you will get results similar to these (10k connections):

Echo performance: 25.5599 echoes/ms

Echo performance: 99.9217 echoes/ms

That is,

* ```node-lws``` echoes messages in less than 30% the time compared to ```ws```.
