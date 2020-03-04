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

/* Run once for every Redis message */
void onMessage(redisAsyncContext *c, void *reply, void *privdata) {
    redisReply *r = (redisReply *) reply;
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
                /* This only means we are subscribed and connected to Redis, debug text should be removed */
                printf("We are subscribed now!\n");
            }
        }
    }
}

/* Todo: make this prettier */
void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
}

/* Todo: make this try to reconnect */
void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Disconnected...\n");
}

int main() {
    /* Read some environment variables */

    /* REDIS_TOPIC is the only required environment variable */
    const char *redisTopic = getenv("REDIS_TOPIC");
    if (!redisTopic) {
        printf("Error: REDIS_TOPIC not set!\n");
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
    uv_loop_t *loop = uv_default_loop();

    /* Hook up uWS with external libuv loop */
    uWS::Loop::get(loop);

    /* Connect to Redis, subscribe to topic */
    redisAsyncContext *c = redisAsyncConnect(redisHostname, redisPort);
    if (c->err) {
        /* Todo: make prettier */
        printf("error: %s\n", c->errstr);
        return 1;
    }

    uWS::App *app = new uWS::App();

    redisLibuvAttach(c, loop);
    redisAsyncSetConnectCallback(c, connectCallback);
    redisAsyncSetDisconnectCallback(c, disconnectCallback);

    std::string subscribeCommand = "SUBSCRIBE " + std::string(redisTopic);
    redisAsyncCommand(c, onMessage, app, subscribeCommand.c_str());

    /* We don't need any per socket data */
    struct PerSocketData {

    };

    /* Define websocket route */
    app->ws<PerSocketData>(path, {
        /* Shared compressor */
        .compression = uWS::SHARED_COMPRESSOR,

        /* We don't expect any messages, we are a megaphone */
        .maxPayloadLength = 0,

        /* We expect clients to ping us from time to time */
        /* Or we ping them, I don't know yet */
        .idleTimeout = 160,

        /* 100kb then you're cut from the team */
        .maxBackpressure = 100 * 1024,
        /* Handlers */
        .open = [redisTopic](auto *ws, auto *req) {
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
