#ifndef GTS_CORE_T15PY_H
#define GTS_CORE_T15PY_H

#include "json.hpp"

#include <WinSock2.h>
#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <thread>
#include <atomic>

using Json = nlohmann::json;

class Diagnostics;

class T15{
private:
    Diagnostics* diag;

    //std::jthread worker;
    std::jthread thread;


    std::atomic<bool> init = false;
    std::atomic<bool> armed = false;
    std::atomic<bool> requestArm = false;
    std::atomic<bool> requestDisarm = false;
    std::atomic<unsigned short> count = 0;
    std::atomic<long long> timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();


public:
    explicit T15(Diagnostics* parent): diag{parent} {};
    ~T15();

    void connect();

    Json handleRequest(Json& request);

    Json status();

    void arm();

    void disarm();
};


#endif //GTS_CORE_T15PY_H
