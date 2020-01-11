#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "hiredis/adapters/libuv.h"

#include "App.h"

#include <uv.h>

const int MESSAGE_LENGTH = 7;
const int SUBSCRIBE_LENGTH = 9;


void onMessage(redisAsyncContext *c, void *reply, void *privdata) {
    redisReply *r = (redisReply *) reply;
    if (reply == NULL) return;

    if (r->type == REDIS_REPLY_ARRAY) {

        /* We always expect action, topic, message */
        if (r->elements == 3) {

            
            if (r->element[0]->len == MESSAGE_LENGTH) {


                std::string_view topic(r->element[1]->str, r->element[1]->len);
                std::string_view message(r->element[2]->str, r->element[2]->len);

                std::cout << "Topic: " << topic << " published: " << message << std::endl;


                ((uWS::App *) privdata)->publish(topic, message, uWS::OpCode::TEXT, true);

            } else if (r->element[0]->len == SUBSCRIBE_LENGTH) {

                printf("We are subscribed now!\n");

            }


        }
    }
}

void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Disconnected...\n");
}

int main() {
    /* Create libuv loop */
    signal(SIGPIPE, SIG_IGN);
    uv_loop_t *loop = uv_default_loop();

    /* Hook up uWS with external libuv loop */
    uWS::Loop::get(loop);

    /* Connect to Redis, subscribe to topic */
    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    if (c->err) {
        printf("error: %s\n", c->errstr);
        return 1;
    }

    uWS::App *app = new uWS::App();

    redisLibuvAttach(c, loop);
    redisAsyncSetConnectCallback(c, connectCallback);
    redisAsyncSetDisconnectCallback(c, disconnectCallback);
    redisAsyncCommand(c, onMessage, app, "SUBSCRIBE trades");

    struct us_listen_socket_t *listen_socket;

    /* ws->getUserData returns one of these */
    struct PerSocketData {

    };

    /* Define websocket route */
    app->ws<PerSocketData>("/*", {
        /* Shared compressor */
        .compression = uWS::SHARED_COMPRESSOR,

        /* We don't expect any messages */
        .maxPayloadLength = 0,

        /* We expect clients to ping us from time to time */
        /* Or we ping them, I don't know yet */
        .idleTimeout = 160,

        /* 100kb then you're cut from the team */
        .maxBackpressure = 100 * 1024,
        /* Handlers */
        .open = [](auto *ws, auto *req) {
            ws->subscribe("trades");
        },
        .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {

        },
        .drain = [](auto *ws) {

        },
        .ping = [](auto *ws) {

        },
        .pong = [](auto *ws) {

        },
        .close = [](auto *ws, int code, std::string_view message) {

        }
    }).listen(9001, [](auto *token) {
        //listen_socket = token;
        if (token) {
            std::cout << "Listening on port " << 9001 << std::endl;
        }
    }).run();
}