#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "App.h"
#include "k.h"
#include "SPSCQueue.hpp"
#include <chrono>
#include <unordered_set>

/* We use libuv to share the same event loop across kdb and uSockets */
/* It could also be possible to use uSockets own event loop, but libuv works fine */
#include <uv.h>

/* No need to do string comparison when we know the exact length of the different actions */
const int MESSAGE_LENGTH = 7;
const int SUBSCRIBE_LENGTH = 9;
const int PSUBSCRIBE_LENGTH = 10;

const char *Topics;

/* Init zero-allocation rapidjson parsing */
#include "rapidjson.h"
#include "document.h"

#include "writer.h"
#include "stringbuffer.h"

using namespace rapidjson;
typedef GenericDocument<UTF8<>, MemoryPoolAllocator<>, MemoryPoolAllocator<>> DocumentType;
char valueBuffer[4096];
char parseBuffer[1024];
MemoryPoolAllocator<> valueAllocator(valueBuffer, sizeof(valueBuffer));
MemoryPoolAllocator<> parseAllocator(parseBuffer, sizeof(parseBuffer));

int missingPongs;
uint64_t lastPong;
uWS::App* global;

/* Set of valid topics */
std::unordered_set<std::string_view> validTopics;

/* Checks if its the right kdb table */
I KdbShape(K x, K tableName, int columnCount) { 
    K columns;
    if(x->t != 0 || x->n != 3)
        return 0;
    // check the table name 
    if(kK(x)[1]->s != tableName->s)
        return 0;
    // check that number of columns>=4 
    columns= kK(kK(x)[2]->k)[0];
    if(columns->n != columnCount)
        return 0;
    // you can add more checks here to ensure that types are as expected
    return 1;
}

I handleOk(I handle) {
    if (handle > 0)
        return 1;
    if (handle == 0) {
        std::cout << "[kdb+] Authentication error : {}" << handle << std::endl;
    } else if(handle == -1) {
        std::cout << "[kdb+] Connection error : {}" << handle << std::endl;
    } else if(handle == -2) {
        std::cout << "[kdb+] Timeout error : {}" << handle << std::endl;
    } 
    return handle;
}


/* data to share in event loop */
struct MetaData {
    int handle; // kdb connection
};

int main() {
    std::cout << "Deployed Logs 3" << std::endl;
    /* TOPICS is the only required environment variable */
    Topics = getenv("TOPICS");
    if (!Topics) {
        printf("Error: WS TOPICS environment variable not set\n");
        return -1;
    }

    /* Put all allowed topics in a hash for quick validation */
    std::string_view validTopicsWs(Topics);
    while (validTopicsWs.length()) {
        auto end = validTopicsWs.find(" ");
        std::string_view topic = validTopicsWs.substr(0, end);

        std::cout << "Using <" << topic << "> as valid topic" << std::endl;

        validTopics.insert(topic);
        if (end == std::string_view::npos) {
            break;
        }
        validTopicsWs.remove_prefix(end + 1);
    }

    /* Read some KDB environment variables */
    const char *kdbHost = getenv("KDB_HOST");
    std::string hostStr(kdbHost);
    int kdbPort = atoi(getenv("KDB_PORT"));

    I handle= khpu(hostStr.data(), kdbPort, "kdb:pass");
    if(!handleOk(handle))
        return EXIT_FAILURE; 
    else
        std::cout << "KDB succesfully Connected" << "\n";

    /* Default port is 4001 */
    char *portString = getenv("SERVICE_PORT");
    int port = portString ? atoi(portString) : 40010;

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

    /* Temporary variable for holding comma separated subscribe list */
    thread_local std::string_view globalTopicsList;

    /* We don't need any per socket data */
    struct PerSocketData {

    };

    /* Hook up uWS with external libuv loop */
    uWS::Loop::get(uv_default_loop());
    uWS::App app;
    /* Define websocket route */
    app.ws<PerSocketData>(path, {
        /* Shared compressor */
        .compression = uWS::SHARED_COMPRESSOR,

        /* We only expect small messages (subscribe, unsubscribe) */
        .maxPayloadLength = 1024,

        /* We expect clients to ping us from time to time */
        /* Or we ping them, I don't know yet */
        /* TODO make it an env var */
        .idleTimeout = 6000,

        /* 100kb then you're skipped until either cut or up to date */
        /* TODO make it an env var */
        .maxBackpressure = 100 * 1024,
        /* Handlers */
        .upgrade = [](auto *res, auto *req, auto *context) {
            /* Upgrading below is immediate, so we can store topics in a global pointer */
            globalTopicsList = req->getQuery("subscribe");

            /* This will immediately emit open event, so we can use globalTopicsList in there */
            res->template upgrade<PerSocketData>({
                /* We initialize PerSocketData struct here */
            }, req->getHeader("sec-websocket-key"),
                req->getHeader("sec-websocket-protocol"),
                req->getHeader("sec-websocket-extensions"),
                context);
        },
        .open = [](auto *ws) {
            /* For all topics we specified, subscribe */
            while (globalTopicsList.length()) {
                auto end = globalTopicsList.find(",");
                std::string_view topic = globalTopicsList.substr(0, end);

                /* Is this even a valid topic? Ignore all topics that mismatches.
                 * It could possibly close the connection otherwise */
                if (validTopics.count(topic)) {
                    ws->subscribe(topic);
                } else {
                    /* Should we close here? */
                }

                if (end == std::string_view::npos) {
                    break;
                }
                globalTopicsList.remove_prefix(end + 1);
            }
        },
        .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {
            /* Parse JSON here and subscribe, unsubscribe */
            /* Example input: {"op": "subscribe", "args": ["orderBookL2_25:XBTUSD"]} */

            /* These documents are pre-allocated */
            DocumentType d(&valueAllocator, sizeof(parseBuffer), &parseAllocator);
            ParseResult ok = d.Parse(message.data(), message.length());

            if (!ok || !d.IsObject()) {
                /* Not the json we want */
                return;
            }

            /* First we get subscribe or unsubscribe */
            Value::ConstMemberIterator opItr = d.FindMember("op");
            if (opItr == d.MemberEnd() || !opItr->value.IsString()) {
                return;
            } else {
                std::string_view op(opItr->value.GetString(), opItr->value.GetStringLength());

                /* We have an op, so get a ref to the args array before we begin */
                Value::ConstMemberIterator argsItr = d.FindMember("args");
                if (argsItr == d.MemberEnd() || !argsItr->value.IsArray()) {
                    return;
                }

                if (op == "subscribe") {
                    for (auto &m : argsItr->value.GetArray()) {
                        /* Is this a valid topic that we allow? */
                        if (m.IsString() && validTopics.count(m.GetString())) {
                            ws->subscribe(m.GetString());
                        }
                    }
                } else if (op == "unsubscribe") {
                    for (auto &m : argsItr->value.GetArray()) {
                        /* Is this a valid topic that we allow? */
                        if (m.IsString() && validTopics.count(m.GetString())) {
                            ws->unsubscribe(m.GetString());
                        }
                    }
                }
            }
        },
        .drain = nullptr,
        /* Respond with last pong from Redis on websocket pings */
        .ping = [](auto *ws, std::string_view message) {
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
    });

    auto meta = MetaData{.handle=handle};
    uv_idle_t idler{.data = &meta};
    uv_idle_init(uv_default_loop(), &idler);
    uv_idle_start(&idler, [](uv_idle_t* handle) {
        
        const auto &obj = (MetaData*)handle->data;

        K response = k(obj->handle,  ".u.sub[`;`]", (K)0);
        
        if(!response) {
            return;
        }
        
        K tradeTableName = ks("trades");
        K depth_l2_full = ks("depthL2");
        K depth_l2_10 = ks("depthL2_10");
        K depth_l2_25 = ks("depthL2_25");
        K depth_l2_50 = ks("depthL2_50");
        K depth_l2_100 = ks("depthL2_100");
        K depth_l3 = ks("depthL3");
        K l3_events = ks("l3_events");
        K ticker = ks("ticker");
        K l2_events = ks("l2_events");

        if(KdbShape(response, depth_l2_10, 8)) { 
            std::cout << "Processing depth_l2_10 table " << std::endl;
            K table= kK(response)[2]->k;
            K columnNames= kK(table)[0]; 
            K columnValues= kK(table)[1]; 

            StringBuffer s;
            Writer<StringBuffer> writer{s};

            writer.StartObject();
            writer.Key("table");
            writer.String("depthL2");
            writer.Key("action");
            writer.String("snapshot");
            writer.Key("data");
            writer.StartObject();            
            writer.Key("instrument");
            std::string instrument = kS(kK(columnValues)[4])[0]; // instrument : symbol
            writer.String(instrument.c_str());
            writer.Key("sequence");
            writer.Uint(kJ(kK(columnValues)[6])[0]);
            writer.Key("is_frozen");
            writer.Bool(false);
            writer.Key("bids");
            writer.StartArray();
            // bids
            for(int i= 0; i < kK(columnValues)[0]->n; i++) {
                bool is_bid = kG(kK(columnValues)[3])[i]; // is_bid : bool
                std::string price = kS(kK(columnValues)[1])[i]; // price : symbol
                std::string qty = kS(kK(columnValues)[2])[i]; // qty : symbol
                std::string instrument = kS(kK(columnValues)[4])[i]; // instrument : symbol
                std::string value = kS(kK(columnValues)[5])[i]; // values : symbol
                long sequence = kJ(kK(columnValues)[6])[i]; // sequence : long
                long timestamp = kJ(kK(columnValues)[7])[i]; // timestamp : long
                if(is_bid) {
                    writer.StartArray();
                    writer.String(price.c_str());
                    writer.String(qty.c_str());
                    writer.EndArray();
                }
            }
            writer.EndArray();
            writer.Key("asks");
            writer.StartArray();
            // asks
            for(int i= 0; i < kK(columnValues)[0]->n; i++) {
                bool is_bid = kG(kK(columnValues)[3])[i]; // is_bid : bool
                std::string price = kS(kK(columnValues)[1])[i]; // price : symbol
                std::string qty = kS(kK(columnValues)[2])[i]; // qty : symbol
                std::string instrument = kS(kK(columnValues)[4])[i]; // instrument : symbol
                std::string value = kS(kK(columnValues)[5])[i]; // values : symbol
                long sequence = kJ(kK(columnValues)[6])[i]; // sequence : long
                long timestamp = kJ(kK(columnValues)[7])[i]; // timestamp : long
                if(!is_bid) {
                    writer.StartArray();
                    writer.String(price.c_str());
                    writer.String(qty.c_str());
                    writer.EndArray();
                }
            }
            writer.EndArray();
            writer.EndObject();
            writer.EndObject();
            auto depthl2json = s.GetString();
            auto topic = std::string("depthL2_10:").append(instrument);
            global->publish(topic, depthl2json, uWS::OpCode::TEXT, true);
            s.Clear();
        } 
        if(KdbShape(response, depth_l2_25, 8)) { 
            std::cout << "Processing depth_l2_25 table " << std::endl;
            K table= kK(response)[2]->k;
            K columnNames= kK(table)[0]; 
            K columnValues= kK(table)[1]; 

            StringBuffer s;
            Writer<StringBuffer> writer{s};

            writer.StartObject();
            writer.Key("table");
            writer.String("depthL2");
            writer.Key("action");
            writer.String("snapshot");
            writer.Key("data");
            writer.StartObject();            
            writer.Key("instrument");
            std::string instrument = kS(kK(columnValues)[4])[0]; // instrument : symbol
            writer.String(instrument.c_str());
            writer.Key("sequence");
            writer.Uint(kJ(kK(columnValues)[6])[0]);
            writer.Key("is_frozen");
            writer.Bool(false);
            writer.Key("bids");
            writer.StartArray();
            // bids
            for(int i= 0; i < kK(columnValues)[0]->n; i++) {
                bool is_bid = kG(kK(columnValues)[3])[i]; // is_bid : bool
                std::string price = kS(kK(columnValues)[1])[i]; // price : symbol
                std::string qty = kS(kK(columnValues)[2])[i]; // qty : symbol
                std::string instrument = kS(kK(columnValues)[4])[i]; // instrument : symbol
                std::string value = kS(kK(columnValues)[5])[i]; // values : symbol
                long sequence = kJ(kK(columnValues)[6])[i]; // sequence : long
                long timestamp = kJ(kK(columnValues)[7])[i]; // timestamp : long

                if(is_bid) {
                    writer.StartArray();
                    writer.String(price.c_str());
                    writer.String(qty.c_str());
                    writer.EndArray();
                }
            }
            writer.EndArray();
            writer.Key("asks");
            writer.StartArray();
            // asks
            for(int i= 0; i < kK(columnValues)[0]->n; i++) {
                bool is_bid = kG(kK(columnValues)[3])[i]; // is_bid : bool
                std::string price = kS(kK(columnValues)[1])[i]; // price : symbol
                std::string qty = kS(kK(columnValues)[2])[i]; // qty : symbol
                std::string instrument = kS(kK(columnValues)[4])[i]; // instrument : symbol
                std::string value = kS(kK(columnValues)[5])[i]; // values : symbol
                long sequence = kJ(kK(columnValues)[6])[i]; // sequence : long
                long timestamp = kJ(kK(columnValues)[7])[i]; // timestamp : long
                if(!is_bid) {
                    writer.StartArray();
                    writer.String(price.c_str());
                    writer.String(qty.c_str());
                    writer.EndArray();
                }
            }
            writer.EndArray();
            writer.EndObject();
            writer.EndObject();
            auto depthl2json = s.GetString();
            auto topic = std::string("depthL2_25:").append(instrument);
            global->publish(topic, depthl2json, uWS::OpCode::TEXT, true);
            s.Clear();
        }
        if(KdbShape(response, depth_l2_50, 8)) { 
            std::cout << "Processing depth_l2_50 table " << std::endl;
            K table= kK(response)[2]->k;
            K columnNames= kK(table)[0]; 
            K columnValues= kK(table)[1]; 

            StringBuffer s;
            Writer<StringBuffer> writer{s};

            writer.StartObject();
            writer.Key("table");
            writer.String("depthL2");
            writer.Key("action");
            writer.String("snapshot");
            writer.Key("data");
            writer.StartObject();            
            writer.Key("instrument");
            std::string instrument = kS(kK(columnValues)[4])[0]; // instrument : symbol
            writer.String(instrument.c_str());
            writer.Key("sequence");
            writer.Uint(kJ(kK(columnValues)[6])[0]);
            writer.Key("is_frozen");
            writer.Bool(false);
            writer.Key("bids");
            writer.StartArray();
            // bids
            for(int i= 0; i < kK(columnValues)[0]->n; i++) {
                bool is_bid = kG(kK(columnValues)[3])[i]; // is_bid : bool
                std::string price = kS(kK(columnValues)[1])[i]; // price : symbol
                std::string qty = kS(kK(columnValues)[2])[i]; // qty : symbol
                std::string instrument = kS(kK(columnValues)[4])[i]; // instrument : symbol
                std::string value = kS(kK(columnValues)[5])[i]; // values : symbol
                long sequence = kJ(kK(columnValues)[6])[i]; // sequence : long
                long timestamp = kJ(kK(columnValues)[7])[i]; // timestamp : long

                if(is_bid) {
                    writer.StartArray();
                    writer.String(price.c_str());
                    writer.String(qty.c_str());
                    writer.EndArray();
                }
            }
            writer.EndArray();
            writer.Key("asks");
            writer.StartArray();
            // asks
            for(int i= 0; i < kK(columnValues)[0]->n; i++) {
                bool is_bid = kG(kK(columnValues)[3])[i]; // is_bid : bool
                std::string price = kS(kK(columnValues)[1])[i]; // price : symbol
                std::string qty = kS(kK(columnValues)[2])[i]; // qty : symbol
                std::string instrument = kS(kK(columnValues)[4])[i]; // instrument : symbol
                std::string value = kS(kK(columnValues)[5])[i]; // values : symbol
                long sequence = kJ(kK(columnValues)[6])[i]; // sequence : long
                long timestamp = kJ(kK(columnValues)[7])[i]; // timestamp : long
                if(!is_bid) {
                    writer.StartArray();
                    writer.String(price.c_str());
                    writer.String(qty.c_str());
                    writer.EndArray();
                }
            }
            writer.EndArray();
            writer.EndObject();
            writer.EndObject();
            auto depthl2json = s.GetString();
            auto topic = std::string("depthL2_50:").append(instrument);
            global->publish(topic, depthl2json, uWS::OpCode::TEXT, true);
            s.Clear();
        }  
        if(KdbShape(response, depth_l2_100, 8)) { 
            std::cout << "Processing depth_l2_100 table " << std::endl;
            K table= kK(response)[2]->k;
            K columnNames= kK(table)[0]; 
            K columnValues= kK(table)[1]; 

            StringBuffer s;
            Writer<StringBuffer> writer{s};

            writer.StartObject();
            writer.Key("table");
            writer.String("depthL2");
            writer.Key("action");
            writer.String("snapshot");
            writer.Key("data");
            writer.StartObject();            
            writer.Key("instrument");
            std::string instrument = kS(kK(columnValues)[4])[0]; // instrument : symbol
            writer.String(instrument.c_str());
            writer.Key("sequence");
            writer.Uint(kJ(kK(columnValues)[6])[0]);
            writer.Key("is_frozen");
            writer.Bool(false);
            writer.Key("bids");
            writer.StartArray();
            // bids
            for(int i= 0; i < kK(columnValues)[0]->n; i++) {
                bool is_bid = kG(kK(columnValues)[3])[i]; // is_bid : bool
                std::string price = kS(kK(columnValues)[1])[i]; // price : symbol
                std::string qty = kS(kK(columnValues)[2])[i]; // qty : symbol
                std::string instrument = kS(kK(columnValues)[4])[i]; // instrument : symbol
                std::string value = kS(kK(columnValues)[5])[i]; // values : symbol
                long sequence = kJ(kK(columnValues)[6])[i]; // sequence : long
                long timestamp = kJ(kK(columnValues)[7])[i]; // timestamp : long

                if(is_bid) {
                    writer.StartArray();
                    writer.String(price.c_str());
                    writer.String(qty.c_str());
                    writer.EndArray();
                }
            }
            writer.EndArray();
            writer.Key("asks");
            writer.StartArray();
            // asks
            for(int i= 0; i < kK(columnValues)[0]->n; i++) {
                bool is_bid = kG(kK(columnValues)[3])[i]; // is_bid : bool
                std::string price = kS(kK(columnValues)[1])[i]; // price : symbol
                std::string qty = kS(kK(columnValues)[2])[i]; // qty : symbol
                std::string instrument = kS(kK(columnValues)[4])[i]; // instrument : symbol
                std::string value = kS(kK(columnValues)[5])[i]; // values : symbol
                long sequence = kJ(kK(columnValues)[6])[i]; // sequence : long
                long timestamp = kJ(kK(columnValues)[7])[i]; // timestamp : long
                if(!is_bid) {
                    writer.StartArray();
                    writer.String(price.c_str());
                    writer.String(qty.c_str());
                    writer.EndArray();
                }
            }
            writer.EndArray();
            writer.EndObject();
            writer.EndObject();
            auto depthl2json = s.GetString();
            auto topic = std::string("depthL2_100:").append(instrument);
            global->publish(topic, depthl2json, uWS::OpCode::TEXT, true);
            s.Clear();
        } 
        if(KdbShape(response, depth_l2_full, 8)) { 
            std::cout << "Processing depth_l2_full table " << std::endl;
            K table= kK(response)[2]->k;
            K columnNames= kK(table)[0]; 
            K columnValues= kK(table)[1]; 

            StringBuffer s;
            Writer<StringBuffer> writer{s};

            writer.StartObject();
            writer.Key("table");
            writer.String("depthL2");
            writer.Key("action");
            writer.String("snapshot");
            writer.Key("data");
            writer.StartObject();            
            writer.Key("instrument");
            std::string instrument = kS(kK(columnValues)[4])[0]; // instrument : symbol
            writer.String(instrument.c_str());
            writer.Key("sequence");
            writer.Uint(kJ(kK(columnValues)[6])[0]);
            writer.Key("is_frozen");
            writer.Bool(false);
            writer.Key("bids");
            writer.StartArray();
            // bids
            for(int i= 0; i < kK(columnValues)[0]->n; i++) {
                bool is_bid = kG(kK(columnValues)[3])[i]; // is_bid : bool
                std::string price = kS(kK(columnValues)[1])[i]; // price : symbol
                std::string qty = kS(kK(columnValues)[2])[i]; // qty : symbol
                std::string instrument = kS(kK(columnValues)[4])[i]; // instrument : symbol
                std::string value = kS(kK(columnValues)[5])[i]; // values : symbol
                long sequence = kJ(kK(columnValues)[6])[i]; // sequence : long
                long timestamp = kJ(kK(columnValues)[7])[i]; // timestamp : long

                if(is_bid) {
                    writer.StartArray();
                    writer.String(price.c_str());
                    writer.String(qty.c_str());
                    writer.EndArray();
                }
            }
            writer.EndArray();
            writer.Key("asks");
            writer.StartArray();
            // asks
            for(int i= 0; i < kK(columnValues)[0]->n; i++) {
                bool is_bid = kG(kK(columnValues)[3])[i]; // is_bid : bool
                std::string price = kS(kK(columnValues)[1])[i]; // price : symbol
                std::string qty = kS(kK(columnValues)[2])[i]; // qty : symbol
                std::string instrument = kS(kK(columnValues)[4])[i]; // instrument : symbol
                std::string value = kS(kK(columnValues)[5])[i]; // values : symbol
                long sequence = kJ(kK(columnValues)[6])[i]; // sequence : long
                long timestamp = kJ(kK(columnValues)[7])[i]; // timestamp : long
                if(!is_bid) {
                    writer.StartArray();
                    writer.String(price.c_str());
                    writer.String(qty.c_str());
                    writer.EndArray();
                }
            }
            writer.EndArray();
            writer.EndObject();
            writer.EndObject();
            auto depthl2json = s.GetString();
            auto topic = std::string("depthL2:").append(instrument);
            global->publish(topic, depthl2json, uWS::OpCode::TEXT, true);
            s.Clear();
        } 
        if(KdbShape(response, l3_events, 9)) { 
            std::cout << "Processing L3_EVENTS table " << std::endl;
            
            K table= kK(response)[2]->k;
            K columnNames= kK(table)[0]; 
            K columnValues= kK(table)[1]; 

            StringBuffer s;
            Writer<StringBuffer> writer(s);


            for(int i= 0; i < kK(columnValues)[0]->n; i++) {
                std::string price = kS(kK(columnValues)[1])[i]; // price : symbol
                std::string qty = kS(kK(columnValues)[2])[i]; // qty : symbol
                bool is_bid = kG(kK(columnValues)[3])[i]; // is_bid : bool
                std::string instrument = kS(kK(columnValues)[4])[i]; // instrument : symbol
                long sequence = kJ(kK(columnValues)[5])[i]; // sequence : long
                long type = kH(kK(columnValues)[6])[i]; // type : short
                std::string order_id = kS(kK(columnValues)[7])[i]; // order id : symbol
                long timestamp = kJ(kK(columnValues)[8])[i]; // timestamp : long

                writer.StartObject();
                writer.Key("table");
                writer.String("depthL3");

                writer.Key("action");
                writer.String("update");

                writer.Key("data");

                writer.StartObject();
                    writer.Key("instrument");
                    writer.String(instrument.c_str());

                    writer.Key("order_id");
                    writer.String(order_id.c_str());


                    writer.Key("side");
                    writer.Uint(is_bid);

                    writer.Key("price");
                    writer.String(price.c_str());

                    writer.Key("qty");
                    writer.String(qty.c_str());

                    writer.Key("order_type");
                    writer.Uint(type);

                    writer.Key("timestamp");
                    writer.Uint(timestamp);


                    writer.Key("sequence");
                    writer.Uint(sequence);

                    writer.EndObject();
                    writer.EndObject();
                
                auto depthL3EventsJson = s.GetString();
                auto topic = std::string("L3_EVENTS:").append(instrument);
                global->publish(topic, depthL3EventsJson, uWS::OpCode::TEXT, true);
                s.Clear();
            }
        }
        if(KdbShape(response, tradeTableName, 7)) { 
            std::cout << "Processing Trade table " << std::endl;
            StringBuffer s;
            Writer<StringBuffer> writer(s);

            K table= kK(response)[2]->k;
            K columnNames= kK(table)[0]; 
            K columnValues= kK(table)[1]; 

            for(int i= 0; i < kK(columnValues)[0]->n; i++) {
                std::string price = kS(kK(columnValues)[1])[i]; // price : symbol
                std::string qty = kS(kK(columnValues)[2])[i]; // qty : symbol
                std::string value = kS(kK(columnValues)[3])[i]; // values : symbol
                bool is_bid = kG(kK(columnValues)[4])[i]; // is_bid : bool
                std::string instrument = kS(kK(columnValues)[5])[i]; // instrument : symbol
                long timestamp = kJ(kK(columnValues)[6])[i]; // timestamp : long
                int volume = 10;

                writer.StartObject();
                writer.Key("table");
                auto tablesName = std::string("trades:").append(instrument);
                auto tn = tablesName.c_str();
                writer.String(tn);

                writer.Key("action");
                writer.String("insert");

                writer.Key("data");

                writer.StartObject();
                    writer.Key("instrument");
                    writer.String(instrument.c_str());

                    writer.Key("price");
                    writer.String(price.c_str());
                    
                    writer.Key("volume");
                    writer.Uint(volume);

                                    
                    writer.Key("values");
                    writer.String(value.c_str());
                                    
                    writer.Key("side");
                    writer.Uint(is_bid);
                                    
                    writer.Key("timestamp");
                    writer.Uint(timestamp);

                writer.EndObject();
                writer.EndObject();

                auto tradeJson = s.GetString();
                auto topic = std::string("trades:").append(instrument);
                global->publish(topic, tradeJson, uWS::OpCode::TEXT, true);
                s.Clear();
            }
        }
        if(KdbShape(response, depth_l3, 8)) { 
            std::cout << "Processing depthL3 table " << std::endl;
            K table= kK(response)[2]->k;
            K columnNames= kK(table)[0]; 
            K columnValues= kK(table)[1]; 

            StringBuffer s;
            Writer<StringBuffer> writer(s);

            writer.StartObject();
            writer.Key("table");
            writer.String("depthL3");
            writer.Key("action");
            writer.String("snapshot");
            writer.Key("data");
            writer.StartObject();
            writer.Key("instrument");
            std::string instrument = kS(kK(columnValues)[4])[0]; // instrument : symbol
            writer.String(instrument.c_str());
            writer.Key("sequence");
            writer.Uint64(kJ(kK(columnValues)[6])[0]);
            writer.Key("bids");
            writer.StartArray();
            for(int i= 0; i < kK(columnValues)[0]->n; i++) {
                std::string price = kS(kK(columnValues)[1])[i]; // price : symbol
                std::string qty = kS(kK(columnValues)[2])[i]; // qty : symbol
                bool is_bid = kG(kK(columnValues)[3])[i]; // is_bid : bool
                std::string instrument = kS(kK(columnValues)[4])[i]; // instrument : symbol
                std::string value = kS(kK(columnValues)[5])[i]; // values : symbol
                long sequence = kJ(kK(columnValues)[6])[i]; // sequence : long
                std::string order_id = kS(kK(columnValues)[7])[i]; // order id : symbol
                long timestamp = kJ(kK(columnValues)[8])[i]; // timestamp : long
                int order_type = 1;
                if(is_bid) {
                    writer.StartArray();
                    writer.String(price.c_str());
                    writer.String(qty.c_str());
                    writer.EndArray();
                }
            }
            writer.EndArray();
            writer.Key("asks");
            writer.StartArray();
            for(int i= 0; i < kK(columnValues)[0]->n; i++) {
                std::string price = kS(kK(columnValues)[1])[i]; // price : symbol
                std::string qty = kS(kK(columnValues)[2])[i]; // qty : symbol
                bool is_bid = kG(kK(columnValues)[3])[i]; // is_bid : bool
                std::string instrument = kS(kK(columnValues)[4])[i]; // instrument : symbol
                std::string value = kS(kK(columnValues)[5])[i]; // values : symbol
                long sequence = kJ(kK(columnValues)[6])[i]; // sequence : long
                std::string order_id = kS(kK(columnValues)[7])[i]; // order id : symbol
                long timestamp = kJ(kK(columnValues)[8])[i]; // timestamp : long
                int order_type = 1;
                if(!is_bid) {
                    writer.StartArray();
                    writer.String(price.c_str());
                    writer.String(qty.c_str());
                    writer.EndArray();
                }
            }
            writer.EndArray();
            writer.EndObject();
            writer.EndObject();
            auto depthL3Json = s.GetString();
            auto topic = std::string("depthL3:").append(instrument);
            global->publish(topic, depthL3Json, uWS::OpCode::TEXT, true);
            s.Clear();
        }
        if(KdbShape(response, l2_events, 7)) { 
            std::cout << "Processing L2_EVENTS table " << std::endl;
            K table= kK(response)[2]->k;
            K columnNames= kK(table)[0]; 
            K columnValues= kK(table)[1];
            
            StringBuffer s;
            Writer<StringBuffer> writer(s);

            writer.StartObject();
            writer.Key("table");
            writer.String("depthL2");
            writer.Key("action");
            writer.String("update");
            writer.Key("data");
            writer.StartObject();

            for(int i= 0; i < kK(columnValues)[0]->n; i++) {
                std::string price = kS(kK(columnValues)[1])[i]; // price : symbol
                std::string qty = kS(kK(columnValues)[2])[i]; // qty : symbol
                bool is_bid = kG(kK(columnValues)[3])[i]; // is_bid : bool
                std::string instrument = kS(kK(columnValues)[4])[i]; // instrument : symbol
                long sequence = kJ(kK(columnValues)[5])[i]; // sequence : long
                int timestamp = kJ(kK(columnValues)[6])[i]; // timestamp

                writer.Key("instrument");
                writer.String(instrument.c_str());
                writer.Key("timestamp");
                writer.Uint(timestamp);
                writer.Key("sequence");
                writer.Uint(sequence);
                writer.Key("price");
                writer.String(price.c_str());
                writer.Key("side");
                writer.Bool(is_bid);
                writer.Key("qty");
                writer.String(qty.c_str());

                writer.EndObject();
                writer.EndObject();

                auto l2Eventsjson = s.GetString();
                auto topic = std::string("L2_EVENTS:").append(instrument);
                global->publish(topic, l2Eventsjson, uWS::OpCode::TEXT, true);
                s.Clear();
            }
        }
        if(response)
            r0(response);
        r0(depth_l2_full);
        r0(depth_l2_10);
        r0(depth_l2_25);
        r0(depth_l2_50);
        r0(depth_l2_100);
        r0(depth_l3);
        r0(l3_events);
        r0(ticker);
        r0(l2_events);
        r0(tradeTableName);
    });
  
    global = &app;
    app.run();
}


