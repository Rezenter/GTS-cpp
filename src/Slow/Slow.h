//
// Created by user on 06.11.2025.
//

#ifndef GTS_CORE_SLOW_H
#define GTS_CORE_SLOW_H

#include <WinSock2.h>
#include "mongoose.h"
#include "json.hpp"
#include <thread>
#include <atomic>

using namespace std::chrono_literals;

using Json = nlohmann::json;

class Diagnostics;

class Slow {
private:
    struct State{
        long long int timestamp_ms;
        bool ok;
    } state;

    Diagnostics* diag;

    std::jthread thread;

    std::atomic<bool> init = false;
    std::atomic<bool> armed = false;
    std::atomic<bool> requestArm = false;
    std::atomic<bool> requestDisarm = false;
    std::atomic<unsigned short> count = 0;
    std::atomic<long long> timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    //char *address = "tcp://192.168.10.47:502";  // Laser cooler ipv4 address
    inline static const char* port = ":3425";
    inline const static char cmdStatus[1] = {0x0c};
    inline const static char cmdArm5V[5]   = {0x02, 0x05, 0x01, 0x2c, 0x02};
    inline const static char cmdArm10V[5]  = {0x02, 0x05, 0x00, 0x2c, 0x02};
    inline const static char cmdSoft5V[5]  = {0x02, 0x04, 0x01, 0x2c, 0x02};
    inline const static char cmdSoft10V[5] = {0x02, 0x04, 0x00, 0x2c, 0x02};

    struct mg_connection *curr_c;
    mg_timer* watchdog;
    static void cfn(struct mg_connection *c, int ev, void *ev_data);
    const unsigned char num;
    static void reconnectSocket(void *arg);

public:
    explicit Slow(Diagnostics* parent, unsigned char num): diag{parent}, num{num} {
        this->connect();
    };

    ~Slow();

    void connect();

    Json status();

    void arm();

    void disarm();
};


#endif //GTS_CORE_SLOW_H
