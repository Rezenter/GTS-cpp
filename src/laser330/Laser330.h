//
// Created by user on 24.09.2024.
//

#ifndef GTS_CORE_LASER330_H
#define GTS_CORE_LASER330_H

#include "mongoose.h"
#include "string"
#include "json.hpp"
#include "Stoppable.h"
#include <queue>
#include <utility>

using Json = nlohmann::json;

class Laser330 : public Stoppable{
private:
    struct mg_mgr* mgr = nullptr;
    inline static const char *address = "tcp://192.168.10.44:4001";  // Lase moxa ipv4 address
    static const size_t MAX_HISTORY_SIZE = 10000;
    bool connected = false;
    mg_timer* watchdog;

    struct mg_connection *curr_c;

    struct State{
        long long int timestamp_ms;
        long long int timeout_ms;
        uint16_t bits;
        uint16_t delayMO;
        uint16_t delayAmp;
        uint8_t state;

    };
    std::deque<State> states;

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

    void connect();
    static void reconnectSocket(void *arg);
    static void cfn(struct mg_connection *c, int ev, void *ev_data);

    //stoppable:
    bool payload() override;
    void beforePayload() override;
    void afterPayload() override;
    long long int lastTimestamp_ms;

public:
    ~Laser330() override;

    Json handleRequest(Json& request);
    void setMgr(mg_mgr* mgr){
        this->mgr = mgr;
    };

    Json status();
    Json setState(Json& req);
};


#endif //GTS_CORE_LASER330_H
