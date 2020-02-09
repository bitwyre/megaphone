# Megaphone distributes events to many web browsers

This project is essentially a Redis-to-WebSocket adapter in the sense that it will publish to WebSocket clients, whatever <string> you publish to Redis.
  
If you connect to redis via `redis-cli` and type `publish trades "some json here"`, this project will send over WebSocket to all connected clients the message "some json here".

This means we could reuse this project for `trades`, `ticker` and `orderbook` without any code change, only a few runtime environment variables changed such as what topic we subscribe to in Redis.

## Usage

Megaphone is a stand-alone executable you launch with a few envionrment variables:

* REDIS_HOST is the address to the main, internal, Redis pub/sub node
* REDIS_PORT is the port to said Redis node
* REDIS_TOPIC is the topic we subscribe to, and listen to events on. What you publish to Redis on this topic will be sent to all WebSockets connected to this Megaphone instance

Example:

`REDIS_HOST=localhost REDIS_PORT=4000 REDIS_TOPIC=trades ./megaphone` launches **one** instance, you may launch as many you want, distributing the load among megaphone instances.

## Building

todo - should build and link statically to libuv
