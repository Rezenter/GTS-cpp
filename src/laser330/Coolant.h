//
// Created by user on 01.10.2024.
//

#ifndef GTS_CORE_COOLANT_H
#define GTS_CORE_COOLANT_H

#include <WinSock2.h>
#include "mongoose.h"
#include "string"
#include "json.hpp"
#include <queue>
#include <utility>

using Json = nlohmann::json;


class Coolant{
private:
    struct mg_mgr* mgr = nullptr;
    inline static const char *address = "tcp://192.168.10.47:502";  // Laser cooler ipv4 address
    inline static const char request[12] = {0x14, 0x69, 0x00, 0x00, 0x00, 0x06, 0x02, 0x04, 0x00, 0x1e, 0x00, 0x02}; //1469 = random 2 bytes, hex representation
    static const size_t MAX_HISTORY_SIZE = 10000;
    mg_timer* watchdog;

    struct mg_connection *curr_c;

    struct State{
        long long int timestamp_ms;
        float temp;
    };
    std::deque<State> states;

    static void reconnectSocket(void *arg);
    static void cfn(struct mg_connection *c, int ev, void *ev_data);

public:
    ~Coolant();

    Json handleRequest(Json& request);
    void setMgr(mg_mgr* mgr){
        this->mgr = mgr;
        this->watchdog = mg_timer_add(this->mgr, 1000, MG_TIMER_REPEAT | MG_TIMER_RUN_NOW, Coolant::reconnectSocket, this);
    };

    Json status();
};


#endif //GTS_CORE_COOLANT_H
