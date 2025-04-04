#ifndef GTS_CORE_OPHIR_H
#define GTS_CORE_OPHIR_H

#include "json.hpp"

#include <WinSock2.h>
#include "include/OphirLMMeasurement.h"
#include <iostream>
#include <iomanip>
#include <Windows.h>
#include <thread>
#include <atomic>

using Json = nlohmann::json;

class Diagnostics;

class Ophir{
private:
    Diagnostics* diag;

    //std::jthread worker;
    std::jthread thread;
    std::wstring serial = L"955794";
    
    long channel = 0;

    std::atomic<bool> init = false;
    std::atomic<bool> armed = false;
    std::atomic<bool> requestArm = false;
    std::atomic<bool> requestDisarm = false;
    std::atomic<unsigned short> count = 0;
    std::atomic<long long> timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    static inline const unsigned short MAX_EVENTS = 16384;


public:
    std::array<double , Ophir::MAX_EVENTS> energy;
    std::array<unsigned long int, Ophir::MAX_EVENTS> times;

    explicit Ophir(Diagnostics* parent): diag{parent} {};
    ~Ophir();

    void connect();

    Json handleRequest(Json& request);

    Json status();

    void arm();

    void disarm();
};


#endif //GTS_CORE_OPHIR_H
