//
// Created by user on 24.09.2024.
//

#ifndef GTS_CORE_LASER330_H
#define GTS_CORE_LASER330_H

#include "mongoose.h"
#include "string"
#include "json.hpp"
#include <thread>
#include <queue>
#include <utility>
#include <mutex>

using Json = nlohmann::json;

class Laser330{
private:
    struct mg_mgr* mgr = nullptr;
    inline static const char *address = "udp://192.168.10.44:4001";  // Laser moxa ipv4 address
    static const size_t MAX_HISTORY_SIZE = 10000;
    bool connected = false;
    mg_timer* watchdog;
    std::jthread worker;
    std::mutex queue_mutex;

    struct mg_connection *curr_c;

    struct State{
        long long int timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        long long int timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        uint16_t bits = 0;
        uint16_t delayMO = 999;
        uint16_t delayAmp = 999;
        uint8_t state = 255;

    };
    std::deque<State> states = {State()}; //to init time

    struct Request{
        uint8_t priority;
        long long int bestBefore;
        std::string request;

        Request(uint8_t priority, long long int bestBefore, std::string request) :
                priority{priority},
                bestBefore{bestBefore},
                request{std::move(request)}
        {};

        friend bool operator< (Request const& l, Request const& r) {
            if(l.priority == r.priority){
                return l.bestBefore < r.bestBefore;
            }
            return l.priority < r.priority;
        }
    };
    std::priority_queue<Request> queue;

    static void reconnectSocket(void *arg);

    long long int lastTimestamp_ms;
    Json setState(Json& req);

public:
    static void cfn(struct mg_connection *c, int ev, void *ev_data);
    ~Laser330();

    Json handleRequest(Json& request);
    void setMgr(mg_mgr* mgr){
        this->mgr = mgr;
        this->watchdog = mg_timer_add(this->mgr, 300, MG_TIMER_REPEAT | MG_TIMER_RUN_NOW, Laser330::reconnectSocket, this);
    };

    Json status();
    void start();
    void stop();
};


#endif //GTS_CORE_LASER330_H
