# Megaphone distributes events to many web browsers

This project is essentially a Redis-to-WebSocket adapter in the sense that it will publish to WebSocket clients, whatever <string> you publish to Redis.
  
If you connect to redis via `redis-cli` and type `publish trades "some json here"`, this project will send over WebSocket to all connected clients the message "some json here".

This means we could reuse this project for `trades`, `ticker` and `orderbook` without any code change, only a few runtime environment variables changed such as what topic we subscribe to in Redis.
