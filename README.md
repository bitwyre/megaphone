<p align="center">
  <a href="https://github.com/bitwyre">
    <img src="https://github.com/bitwyre/interface/blob/develop/public/static/images/logo.png" alt="Logo" height="80" width="auto">
  </a>
</p>

![Workflow Status](https://github.com/bitwyre/megaphone/workflows/CCPP%20CI/badge.svg)

# Megaphone distributes events to many web browsers

## Authors

- [Alex Hultman](https://github.com/alexhultman)

## Description

This project is essentially a Redis-to-WebSocket adapter in the sense that it will publish to WebSocket clients, whatever <string> you publish to Redis.
  
If you connect to redis via `redis-cli` and type `publish trades "some json here"`, this project will send over WebSocket to all connected clients the message "some json here".

This means we could reuse this project for `trades`, `ticker` and `orderbook` without any code change, only a few runtime environment variables changed such as what topic we subscribe to in Redis.

## Building

* `git clone --recursive git@github.com:bitwyre/megaphone.git`
* `make deps -C megaphone` (requires autotools, autoconf, automake for building libuv)
* `make -C megaphone`

Now you should have an executable, megaphone, without any non-standard dependencies to worry about (you can move this executable across different Linux systems without breakage).

## Usage

Megaphone is a stand-alone executable you launch with a few envionrment variables:

* REDIS_HOST is the address, hostname to the main, internal, Redis pub/sub node
* REDIS_PORT is the port to said Redis node
* REDIS_TOPIC is the topic we subscribe to, and listen to events on. What you publish to Redis on this topic will be sent to all WebSockets connected to this Megaphone instance
* SERVICE_IP is the hostname of the WebSocket server
* SERVICE_PORT is the port of the WebSocket server
* SERVICE_PATH is the path of the WebSocket server

Example:

* You need a redis-server running, for instance on `localhost` and port `6379`.

* `REDIS_HOST=localhost REDIS_PORT=6379 REDIS_TOPIC=trades ./megaphone` launches **one** instance, you may launch as many you want, distributing the load among megaphone instances.


* Now you may test by starting `redis-cli` and typing `publish trades "hello world!"`, which should trigger a distribution of "hello world! over all to megaphone connected WebSocket clients.

## Docker Example

```bash
docker run --rm \
    -e "REDIS_TOPIC=pubsub_topic" \
    -e "REDIS_PORT=6379" \
    -e "REDIS_HOST=127.0.0.1" \
    -e "SERVICE_IP=127.0.0.1" \
    -e "SERVICE_PORT=4545" \
    -e "SERVICE_PATH=/ws/topic" \
    --net=host \
    bitwyre/megaphone:latest
```

## Contributors

- [Alex Hultman](https://github.com/alexhultman)
- [Dendi](https://github.com/dendisuhubdy)
- [Kresna](https://github.com/ujang360)

# Copyright

- &copy; 2020 Alex Hultman
- &copy; 2020 Bitwyre Technologies LLC
