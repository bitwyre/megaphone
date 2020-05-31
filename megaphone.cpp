#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "hiredis/adapters/libuv.h"

#include "App.h"

#include <chrono>

/* We use libuv to share the same event loop across hiredis and uSockets */
/* It could also be possible to use uSockets own event loop, but libuv works fine */
#include <uv.h>

/* No need to do string comparison when we know the exact length of the different actions */
const int MESSAGE_LENGTH = 7;
const int SUBSCRIBE_LENGTH = 9;
const int PSUBSCRIBE_LENGTH = 10;

const char *redisHostname;
int redisPort;
const char *redisTopic;

/* We use this to ping Redis */
uv_timer_t pingTimer;
int missingPongs;
uint64_t lastPong;

void onMessage(redisAsyncContext *c, void *reply, void *privdata);

/* Connect to Redis server, manages reconnect by calling itself when needed */
bool connectToRedis(uWS::App *app) {
    /* Connect to Redis, subscribe to topic */
    redisAsyncContext *c = redisAsyncConnect(redisHostname, redisPort);
    /* Failing here is typically failure to allocate resources or probably DNS resolution */
    if (c->err) {
        printf("Failed to allocate connection: %s\n", c->errstr);
        return false;
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

        /* This is a hack to workaround bugs in hiredis, see issue #351 */
        redisAsyncCommand((redisAsyncContext *) c, onMessage, c->data, "PSUBSCRIBE hello");
    });

    redisAsyncSetDisconnectCallback(c, [](const redisAsyncContext *c, int status) {
        printf("Reconnecting to Redis\n");

        /* We are no longer connected to Redis, so stop pinging it */
        pingTimer.data = nullptr;
        missingPongs = 0;

        /* We retry connecting immediately */
        connectToRedis((uWS::App *) c->data);

        /* Old context is always freed on return from here */
    });

    return true;
}

/* Run once for every Redis message */
void onMessage(redisAsyncContext *c, void *reply, void *privdata) {
    redisReply *r = (redisReply *) reply;
    /* Reply is valid for the duration of this callback, yet NULL is passed if errors occurred */
    if (!reply) {
        return;
    }

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

                /* We don't handle this any particular way */

                /* This only means we are subscribed and connected to Redis, debug text should be removed */
                printf("We are connected and subscribed to Redis now\n");

            } else if (r->element[0]->len == PSUBSCRIBE_LENGTH) {
                printf("We are psubscribed to pong now!\n");

                /* Start a timer sending Pings to Redis */
                pingTimer.data = c;
            }
        } else if (r->elements == 2) {
            /* We get {pong, hello} for every ping hello */
            missingPongs = 0;

            /* Update last seen Redis pong */
            lastPong = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
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

    /* Default ping interval is 10s */
    char *redisPingIntervalString = getenv("REDIS_PING_INTERVAL");
    int redisPingInterval = redisPingIntervalString ? atoi(redisPingIntervalString) : 10;

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

        /* Do not send pings unless we are connected and subscribed to pongs */
        if (!c) {
            return;
        }

        if (++missingPongs == 2) {
            /* If we did not get a pong in time, disconnect */
            printf("We did not get pong in time, assuming disconnected\n");
            redisAsyncFree(c);
            return;
        }

        /* This is a hack to get hiredis to report pongs under the psubscribe callback */
        redisAsyncCommand((redisAsyncContext *) c, onMessage, c->data, "PING hello");
    }, redisPingInterval * 1000, redisPingInterval * 1000);

    /* Connect to Redis, this will automatically keep reconnecting once disconnected */
    if (!connectToRedis(&app)) {
        /* If we failed to even allocate connection resources just exit the process */
        return -1;
    }

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
        },
        .message = nullptr,
        .drain = nullptr,
        /* Respond with last pong from Redis on websocket pings */
        .ping = [](auto *ws) {
            ws->send({(char *) &lastPong, 8}, uWS::OpCode::BINARY);
        },
        .pong = nullptr,
        .close = nullptr
    }).listen(hostname, port, [port, hostname](auto *listenSocket) {
        if (listenSocket) {
            std::cout << "Accepting WebSockets on port " << port << ", hostname " << hostname << std::endl;
        } else {
            std::cout << "Error: Failed to listen to port " << port << std::endl;
        }
    }).run();
}
