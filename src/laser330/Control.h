//
// Created by nz on 30.04.2020.
//

#ifndef INC_1064DRIVER_CONTROL_H
#define INC_1064DRIVER_CONTROL_H

#include <pthread.h>
#include "Chatter.h"
#include <functional>
#include <chrono>
#include <future>

class Control {
private:
    bool remote = false;
    char animation[4] = {'|', '/', '|', '\\'};
    uint8_t anim_counter = 0;
    Chatter* chatter;

    bool connected = false;
    bool remoteRequired = false;
    const unsigned short cyclicTimeout = 300; //[ms] pause between cyclic function repetition
    unsigned short readyTimeout = 10; //[s] pause between packets for readyToShoot state hold on
    unsigned short disarmTimeout = 60; //[s] total time in readyToShoot state
    int stateCurrent = State::error;
    int stateRequired = State::idle;
    bool timeLock = false;
    uint16_t currentStatus;
    std::chrono::milliseconds timeout;

    void cyclic();
    void updateState();
    bool remoteRequest();
    bool stateRequest();
    void displayState();

public:
    enum State{
        idle, //state 2
        warmUp, //state 2 to 3 transition 10+ sec
        readyToShoot, //state 3
        shooting, //state 4
        coolDown, //state 3+ to 2 transition
        error //state 1
    };

    enum Status{
        powerBit,
        generatorBit,
        internalSyncBit,
        generatorPowerBit,
        tempOKBit,
        remoteBit,
        desyncBit,
        singleShotBit,
        dev1Bit,
        dev2Bit,
        fireAtWillBit,
        dev3Bit,
        dev4Bit
    };

    bool exit = false;

    Control(Chatter&);
    void setState(State newState);
    bool remoteControl(bool grab);
};


#endif //INC_1064DRIVER_CONTROL_H
