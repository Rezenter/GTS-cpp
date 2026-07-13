//
// Created by user on 23.12.2025.
//

#ifndef GTS_CORE_ALLSLOW_H
#define GTS_CORE_ALLSLOW_H
#include "Diagnostics/Diagnostics.h"
#include <WinSock2.h>
#include "mongoose.h"
#include <thread>
#include <vector>
#include <mutex>
#include "json.hpp"

#include "Slow/Slow.h"


using namespace std::chrono_literals;
using Json = nlohmann::json;

class Diagnostics;

class AllSlow {
private:
    Diagnostics* diag;
    std::jthread thread;
    std::atomic<bool> initialised = false;
    std::atomic<bool> armed = false;
    std::atomic<bool> requestArm = false;
    std::atomic<bool> requestDisarm = false;
    std::atomic<long long> timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    struct mg_mgr* mgr = nullptr;
    std::vector<Slow*> boards;

public:
    std::mutex vectorMutex;

    explicit AllSlow(Diagnostics* parent): diag{parent} {};

    void setMgr(mg_mgr* mgr){
        this->mgr = mgr;
    };


    void init();

    Json handleRequest(Json& request);

    Json status();

    void arm();

    void disarm();
};


#endif //GTS_CORE_ALLSLOW_H
