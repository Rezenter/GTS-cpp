//
// Created by nz on 22.08.2023.
//
#ifndef GTS_CORE_KAPELLMEISTER_H
#define GTS_CORE_KAPELLMEISTER_H
#include "mongoose.h"
#include "json.hpp"
#include <array>
#include <thread>
#include <chrono>
#include <functional>
#include <cstdio>
#include <atomic>

using Json = nlohmann::json;

class Timer {
public:
    ~Timer() {
        if (mRunning) {
            stop();
        }
    }
    typedef std::chrono::milliseconds Interval;
    typedef std::function<void(void)> Timeout;

    void start(const Interval &interval, const Timeout &timeout) {
        mRunning = true;

        mThread = std::thread([this, interval, timeout] {
            while (mRunning) {
                std::this_thread::sleep_for(interval);

                timeout();
            }
        });
    }
    void stop() {
        mRunning = false;
        mThread.join();
    }

private:
    std::thread mThread{};
    std::atomic_bool mRunning{};
};

struct Axes {
    int32_t coordinate_1;
    int32_t coordinate_2;
    unsigned char flags_1;
    unsigned char flags_2;

    [[nodiscard]] bool calibrated_1() const{
        return (flags_1 >> 0) & 1;
    }
    [[nodiscard]] bool stopped_1() const{
        return (flags_1 >> 1) & 1;
    }
    [[nodiscard]] bool limit1_1() const{
        return (flags_1 >> 2) & 1;
    }
    [[nodiscard]] bool limit2_1() const{
        return (flags_1 >> 3) & 1;
    }
    [[nodiscard]] bool isCounterclockwise_1() const{
        return (flags_1 >> 4) & 1;
    }
    [[nodiscard]] unsigned char calibration_1() const{
        return (flags_1 >> 5);
    }

    [[nodiscard]] bool calibrated_2() const{
        return (flags_2 >> 0) & 1;
    }
    [[nodiscard]] bool stopped_2() const{
        return (flags_2 >> 1) & 1;
    }
    [[nodiscard]] bool limit1_2() const{
        return (flags_2 >> 2) & 1;
    }
    [[nodiscard]] bool limit2_2() const{
        return (flags_2 >> 3) & 1;
    }
    [[nodiscard]] bool isCounterclockwise_2() const{
        return (flags_2 >> 4) & 1;
    }
    [[nodiscard]] unsigned char calibration_2() const{
        return (flags_2 >> 5);
    }
};

union Reply{
    uint8_t buffer[10];
    Axes status;
};

class Mirror{
public:
    //std::string address = "udp://192.168.10.70:9";
    std::string address = "udp://192.168.10.71:8888";
    //construct address from last int
    Timer timer;
    struct mg_mgr mgr;
    struct mg_connection *c;
    bool init = false;
    Reply status;
    int32_t span_1 = 0;
    int32_t span_2 = 0;
    unsigned char outBuffer[10];

    Json getStatus(){
        return {
                {"ok", true},
                {"Axis1", {
                               {"calibrated", this->status.status.calibrated_1()},
                               {"stopped", this->status.status.stopped_1()},
                               {"limit1", this->status.status.limit1_1()},
                               {"limit2", this->status.status.limit2_1()},
                               {"isCounterclockwise", this->status.status.isCounterclockwise_1()},
                               {"coordinate", this->status.status.coordinate_1},
                               {"calibration_state", this->status.status.calibration_1()},
                               {"span", this->span_1}
                       }
                },
                {"Axis2", {
                               {"calibrated", this->status.status.calibrated_2()},
                               {"stopped", this->status.status.stopped_2()},
                               {"limit1", this->status.status.limit1_2()},
                               {"limit2", this->status.status.limit2_2()},
                               {"isCounterclockwise", this->status.status.isCounterclockwise_2()},
                               {"coordinate", this->status.status.coordinate_2},
                               {"calibration_state", this->status.status.calibration_2()},
                               {"span", this->span_2}
                       }
                }
        };
    }

    void update();
    void move(int axis, float speed);
    void moveTo(int axis, int32_t pos);
    void stop();
    void calibrate();
};

class Kapellmeister{
private:
    //std::array<std::string, 1> addresses = {"udp://192.168.10.70:8888"};
    inline static Mirror mirror = Mirror();
    static void eventProcessorUDP(struct mg_connection *c, int ev, void *ev_data, void *fn_data);

public:
    Json requestHandler(Json req);
    //bool connect(){return false;};
};

#endif //GTS_CORE_KAPELLMEISTER_H
