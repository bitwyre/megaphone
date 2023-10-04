#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "hiredis/adapters/libuv.h"

#include "App.h"
#include "k.h"
#include "SPSCQueue.hpp"
#include <chrono>
#include <unordered_set>

/* We use libuv to share the same event loop across hiredis and uSockets */
/* It could also be possible to use uSockets own event loop, but libuv works fine */
#include <uv.h>

/* No need to do string comparison when we know the exact length of the different actions */
const int MESSAGE_LENGTH = 7;
const int SUBSCRIBE_LENGTH = 9;
const int PSUBSCRIBE_LENGTH = 10;

const char *redisHostname;
int redisPort;
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

/* We use this to ping Redis */
uv_timer_t pingTimer;
int missingPongs;
uint64_t lastPong;

/* Set of valid topics */
std::unordered_set<std::string_view> validTopics;

struct Payload{
    std::string topic_;
    std::string data_;
};

I shapeOfTrade(K x, K tableName) { 
    K columns;
    // check that second element is a table name 
    if(kK(x)[1]->s != tableName->s)
        return 0;
    // check that number of columns>=4 
    columns = kK(kK(x)[2]->k)[0];
    if(columns->n != 6)
        return 0;
    // you can add more checks here to ensure that types are as expected
    return 1;
}

I shapeOfl3Events(K x, K tableName) { 
    K columns;
    if(kK(x)[1]->s != tableName->s)
        return 0;
    columns= kK(kK(x)[2]->k)[0];
    if(columns->n != 8)
        return 0;
    // you can add more checks here to ensure that types are as expected
    return 1;
}

I shapeOfdepthl3(K x, K tableName) { 
    K columns;
    // check the table name 
    if(kK(x)[1]->s != tableName->s)
        return 0;

    // check that number of columns>=4 
    columns= kK(kK(x)[2]->k)[0];
    if(columns->n != 8)
        return 0;
    // you can add more checks here to ensure that types are as expected
    return 1;
}

I shapeOfl2Events(K x, K tableName) { 
    K columns;
    // check the table name 
    if(kK(x)[1]->s != tableName->s)
        return 0;
    // check that number of columns>=4 
    columns= kK(kK(x)[2]->k)[0];
    if(columns->n != 5)
        return 0;
    // you can add more checks here to ensure that types are as expected
    return 1;
}

I shapeOfdepthl2(K x, K tableName) { 
    K columns;
    // check that second element is a table name 
    if(kK(x)[1]->s != tableName->s)
        return 0;
    // check that number of columns>=4 
    columns= kK(kK(x)[2]->k)[0];
    if(columns->n != 7)
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
int main() {

    rigtorp::SPSCQueue<Payload> wsQueue{1000000};

    /* REDIS_TOPIC is the only required environment variable */
    Topics = getenv("TOPICS");
    if (!Topics) {
        printf("Error: REDIS_TOPIC environment variable not set\n");
        return -1;
    }

    /* Put all allowed topics in a hash for quick validation */
    std::string_view validRedisTopics(Topics);
    while (validRedisTopics.length()) {
        auto end = validRedisTopics.find(" ");
        std::string_view topic = validRedisTopics.substr(0, end);

        std::cout << "Using <" << topic << "> as valid topic" << std::endl;

        validTopics.insert(topic);
        if (end == std::string_view::npos) {
            break;
        }
        validRedisTopics.remove_prefix(end + 1);
    }

    /* Read some environment variables */
    const char *kdbHost = getenv("KDB_HOST");
    std::string hostStr(kdbHost);
    int kdbPort = atoi(getenv("KDB_PORT"));

    I handle= khpu(hostStr.data(), kdbPort, "kdb:pass");
    if(!handleOk(handle))
        return EXIT_FAILURE; 

    std::cout << "kdb+ Succesfully connected" << std::endl;
    
    K response, table, columnNames, columnValues;
    K tradeTableName = ks("trades");
    K depth_l2_full = ks("L2_FULL");
    K depth_l2_10 = ks("L2_10");
    K depth_l2_25 = ks("L2_25");
    K depth_l2_50 = ks("L2_50");
    K depth_l2_100 = ks("L2_100");

    K depth_l3 = ks("depth_l3");
    K l3_events = ks("l3_events");
    K ticker = ks("ticker");
    K l2_events = ks("l2_events");

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

    /* Temporary variable for holding comma separated subscribe list */
    thread_local std::string_view globalTopicsList;

    /* We don't need any per socket data */
    struct PerSocketData {

    };

    std::string tradesTopic = "trades:";
    while(1) {
        // subscribe to all tables from kdb+ tickerplant
        response = k(handle, ".u.sub[`;`]", (K)0); 
        
        if(!response)
            break;

        if(shapeOfTrade(response, tradeTableName)) { 

            StringBuffer s;
            Writer<StringBuffer> writer(s);

            table= kK(response)[2]->k;
            columnNames= kK(table)[0]; 
            columnValues= kK(table)[1]; 

            for(int i= 0; i < kK(columnValues)[0]->n; i++) {
                std::string price = kS(kK(columnValues)[1])[i]; // price : symbol
                std::string qty = kS(kK(columnValues)[2])[i]; // qty : symbol
                std::string value = kS(kK(columnValues)[3])[i]; // values : symbol
                bool is_bid = kG(kK(columnValues)[4])[i]; // is_bid : bool
                std::string instrument = kS(kK(columnValues)[5])[i]; // instrument : symbol
                long timestamp = kJ(kK(columnValues)[6])[i]; // timestamp : long
                int volume = 10;
                std::string id = "the-id-goes-here";

                writer.StartObject();
                writer.Key("table");
                auto tn = std::string("trades:").append(instrument).c_str();
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
                    writer.Uint(volume);
                                    
                    writer.Key("side");
                    writer.Uint(is_bid);

                                    
                    writer.Key("id");
                    writer.String(id.c_str());
                                    
                    writer.Key("timestamp");
                    writer.Uint(timestamp);

                writer.EndObject();
                writer.EndObject();

                auto tradeJson = s.GetString();
                wsQueue.push(Payload{.topic_=tradesTopic+=instrument, .data_=tradeJson});
                //((uWS::App *) privdata)->publish(topic, message, uWS::OpCode::TEXT, true);
                s.Clear();
            }
        }

        if(shapeOfl3Events(response, l3_events)) { 
            table= kK(response)[2]->k;
            columnNames= kK(table)[0]; 
            columnValues= kK(table)[1]; 

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
                writer.String("insert");

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
                    writer.String(instrument.c_str());

                    writer.Key("type");
                    writer.Uint(type);

                    writer.Key("timestamp");
                    writer.Uint(timestamp);


                    writer.Key("sequence");
                    writer.Uint(sequence);

                    writer.EndObject();
                    writer.EndObject();
                
                auto depthL3EventsJson = s.GetString();
                //wsQueue.push(Payload{.topic_="", .data_=depthL3EventsJson});
                s.Clear();
            }
        }

        if(shapeOfdepthl3(response, depth_l3)) { 
            table= kK(response)[2]->k;
            columnNames= kK(table)[0]; 
            columnValues= kK(table)[1]; 

            StringBuffer s;
            Writer<StringBuffer> writer(s);

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


                writer.StartObject();
                writer.Key("table");
                auto tn = std::string("trades:").append(instrument);
                writer.String(tn.c_str());

                                writer.StartObject();
                writer.Key("table");
                writer.String("depthL3");

                writer.Key("action");
                writer.String("insert");

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
                    writer.String(instrument.c_str());

                    writer.Key("type");
                    writer.Uint(order_type);

                    writer.Key("timestamp");
                    writer.Uint(timestamp);


                    writer.Key("sequence");
                    writer.Uint(sequence);

                    writer.Key("values");
                    writer.String(value.c_str());

                    writer.EndObject();
                    writer.EndObject();
                
                auto depthL3Json = s.GetString();
                wsQueue.push(Payload{.topic_="", .data_=depthL3Json});
                s.Clear();
            }
        }

        if(shapeOfl2Events(response, l2_events)) { 
            table= kK(response)[2]->k;
            columnNames= kK(table)[0]; 
            columnValues= kK(table)[1];
            
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
                writer.Key("side");
                writer.Uint(is_bid);
                writer.Key("qty");
                writer.String(qty.c_str());

                writer.EndObject();
                writer.EndObject();

                auto l2Eventsjson = s.GetString();
                wsQueue.push(Payload{.topic_="", .data_=l2Eventsjson});
                s.Clear();
            }
        }

        if(shapeOfdepthl2(response, depth_l2_full)) { 
            table= kK(response)[2]->k;
            columnNames= kK(table)[0]; 
            columnValues= kK(table)[1]; 

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
            //wsQueue.push(Payload{.topic_="", .data_=depthl2json});
            s.Clear();
        }
        if(shapeOfdepthl2(response, depth_l2_10)) { 
            table= kK(response)[2]->k;
            columnNames= kK(table)[0]; 
            columnValues= kK(table)[1]; 

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
            //wsQueue.push(Payload{.topic_="", .data_=depthl2json});
            s.Clear();
        }
        if(shapeOfdepthl2(response, depth_l2_25)) { 
            table= kK(response)[2]->k;
            columnNames= kK(table)[0]; 
            columnValues= kK(table)[1]; 

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
            //wsQueue.push(Payload{.topic_="", .data_=depthl2json});
            s.Clear();
        }
        if(shapeOfdepthl2(response, depth_l2_50)) { 
            table= kK(response)[2]->k;
            columnNames= kK(table)[0]; 
            columnValues= kK(table)[1]; 

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
            //wsQueue.push(Payload{.topic_="", .data_=depthl2json});
            s.Clear();
        }
        if(shapeOfdepthl2(response, depth_l2_100)) { 
            table= kK(response)[2]->k;
            columnNames= kK(table)[0]; 
            columnValues= kK(table)[1]; 

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
            //wsQueue.push(Payload{.topic_="", .data_=depthl2json});
            s.Clear();
        }
        r0(response);
    }

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
    }).run();
}

