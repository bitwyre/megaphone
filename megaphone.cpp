#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "hiredis/adapters/libuv.h"

#include "App.h"

/* We use libuv to share the same event loop across hiredis and uSockets */
/* It could also be possible to use uSockets own event loop, but libuv works fine */
#include <uv.h>

/* No need to do string comparison when we know the exact length of the different actions */
const int MESSAGE_LENGTH = 7;
const int SUBSCRIBE_LENGTH = 9;

const char *redisHostname;
int redisPort;
const char *redisTopic;

/* We use this to ping Redis */
uv_timer_t pingTimer;

void onMessage(redisAsyncContext *c, void *reply, void *privdata);

/* Connect to Redis server, manages reconnect by calling itself when needed */
redisAsyncContext *connectToRedis(uWS::App *app) {
    /* Connect to Redis, subscribe to topic */
    redisAsyncContext *c = redisAsyncConnect(redisHostname, redisPort);
    /* Failing here is typically failure to allocate resources or probably DNS resolution */
    if (c->err) {
        printf("Error redisAsyncConnect: %s\n", c->errstr);
        return nullptr;
    }

    /* User data is the app */
    c->data = app;

    redisLibuvAttach(c, uv_default_loop());
    redisAsyncSetConnectCallback(c, [](const redisAsyncContext *c, int status) {
        /* Failure here is async, such as refused connection or timeout */
        if (status != REDIS_OK) {
            printf("Failed connecting to Redis: %s\n", c->errstr);

            /* We retry connecting immediately */
            connectToRedis((uWS::App *) c->data);

            /* redisAsyncContext is freed by hiredis in this case */
            return;
        }

        /* Once connected, subscribe to our topic */
        std::string subscribeCommand = "SUBSCRIBE " + std::string(redisTopic);
        redisAsyncCommand((redisAsyncContext *) c, onMessage, c->data, subscribeCommand.c_str());
    });

    redisAsyncSetDisconnectCallback(c, [](const redisAsyncContext *c, int status) {
        /* We are no longer connected to Redis, so stop pinging it */
        pingTimer.data = nullptr;

        if (status != REDIS_OK) {
            printf("Lost connection to Redis: %s\n", c->errstr);

            /* We retry connecting immediately */
            connectToRedis((uWS::App *) c->data);

            /* Again, in this case redisAsyncContext is freed by hiredis */
            return;
        }

        /* Here we need to free the redisAsyncContext */
        printf("Redis disconnected cleanly\n");

        /* We retry connecting immediately */
        connectToRedis((uWS::App *) c->data);

        redisAsyncFree((redisAsyncContext *) c);
    });

    /* We never use this actually, no need to return */
    return c;
}

/* Run once for every Redis message */
void onMessage(redisAsyncContext *c, void *reply, void *privdata) {
    redisReply *r = (redisReply *) reply;
    /* If we have no response then just return */
    if (!reply) {
        return;
    }

    /* PONG responses are discarded by hiredis even though they technically reach us */

    if (r->type == REDIS_REPLY_ARRAY) {
        /* We always expect action, topic, message */
        if (r->elements == 3) {
            if (r->element[0]->len == MESSAGE_LENGTH) {
                std::string_view topic(r->element[1]->str, r->element[1]->len);
                std::string_view message(r->element[2]->str, r->element[2]->len);

                /* Debug text for now, should be removed with time */
                std::cout << "Topic: " << topic << " published: " << message << std::endl;

                /* Whatever we are sent from Redis, we distribute to our set of clients */
                ((uWS::App *) privdata)->publish(topic, message, uWS::OpCode::TEXT, true);
            } else if (r->element[0]->len == SUBSCRIBE_LENGTH) {
                /* This only means we are subscribed and connected to Redis, debug text should be removed */
                printf("We are connected and subscribed to Redis now\n");

                /* Start a timer sending Pings to Redis */
                pingTimer.data = c;
            }
        }
    }
}

int main() {
    /* Read some environment variables */

    /* REDIS_TOPIC is the only required environment variable */
    redisTopic = getenv("REDIS_TOPIC");
    if (!redisTopic) {
        printf("Error: REDIS_TOPIC environment variable not set\n");
        return -1;
    }

    /* Default Redis port is 6379 */
    char *redisPortString = getenv("REDIS_PORT");
    redisPort = redisPortString ? atoi(redisPortString) : 6379;

    /* Default Redis hostname is 127.0.0.1 */
    redisHostname = getenv("REDIS_HOST");
    if (!redisHostname) {
        redisHostname = "127.0.0.1";
    }

    /* Default port is 4001 */
    char *portString = getenv("SERVICE_PORT");
    int port = portString ? atoi(portString) : 4001;

    /* Default hostname is 0.0.0.0 */
    const char *hostname = getenv("SERVICE_IP");
    if (!hostname) {
        hostname = "0.0.0.0";
    }

    /* Default route is /* */
    const char *path = getenv("SERVICE_PATH");
    if (!path) {
        path = "/*";
    }

    /* Create libuv loop */
    signal(SIGPIPE, SIG_IGN);

    /* Hook up uWS with external libuv loop */
    uWS::Loop::get(uv_default_loop());
    uWS::App app;

    /* Start a timer sending pings to Redis every 10 seconds */
    uv_timer_init(uv_default_loop(), &pingTimer);
    pingTimer.data = nullptr;
    uv_timer_start(&pingTimer, [](uv_timer_t *t) {
        /* We store currently connected Redis context in user data */
        redisAsyncContext *c = (redisAsyncContext *) t->data;

        /* Redis does respond with a PONG but hiredis bugs discard it so the callback is never called.
         * For us this doesn't matter as we only want to send something to trigger errors on the socket if so */
        redisAsyncCommand((redisAsyncContext *) c, onMessage, c->data, "PING");
    }, 10000, 10000);

    /* Connect to Redis, this will automatically keep reconnecting once disconnected */
    connectToRedis(&app);

    /* We don't need any per socket data */
    struct PerSocketData {

    };

    /* Define websocket route */
    app.ws<PerSocketData>(path, {
        /* Shared compressor */
        .compression = uWS::SHARED_COMPRESSOR,

        /* We don't expect any messages, we are a megaphone */
        .maxPayloadLength = 0,

        /* We expect clients to ping us from time to time */
        /* Or we ping them, I don't know yet */
        .idleTimeout = 160,

        /* 100kb then you're skipped until either cut or up to date */
        .maxBackpressure = 100 * 1024,
        /* Handlers */
        .open = [](auto *ws, auto *req) {
            /* It matters what we subscribe to */
            ws->subscribe(redisTopic);
        }
    }).listen(hostname, port, [port, hostname](auto *listenSocket) {
        if (listenSocket) {
            std::cout << "Accepting WebSockets on port " << port << ", hostname " << hostname << std::endl;
        } else {
            std::cout << "Error: Failed to listen to port " << port << std::endl;
        }
    }).run();
}
