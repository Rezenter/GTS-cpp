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

    std::jthread worker;
    std::wstring serial = L"955794";
    long hDevice = 0;
    long channel = 0;
    unsigned short current_ind = 0;
    
    struct CoInitializer{
        CoInitializer() { CoInitialize(nullptr); }
        ~CoInitializer() { CoUninitialize(); }
    };

    CoInitializer initializer;// must call for COM initialization and deinitialization

    OphirLMMeasurement OphirLM; // COMObject it can be only used directly from the thread it was created in

    void disconnect();

    bool init = false;
    std::atomic<bool> armed = false;

    std::vector<double> values;
    std::vector<double> timestamps;
    std::vector<OphirLMMeasurement::Status> statuses;

public:
    explicit Ophir(Diagnostics* parent): diag{parent} {};
    ~Ophir();

    void connect();

    Json handleRequest(Json& request);

    Json status();

    void arm();

    void disarm();
};


#endif //GTS_CORE_OPHIR_H
