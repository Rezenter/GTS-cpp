//
// Created by user on 24.10.2024.
//

#ifndef GTS_CORE_LINK_H
#define GTS_CORE_LINK_H

#define CAEN_USE_DIGITIZERS
#define IGNORE_DPP_DEPRECATED

#include "include/CAENDigitizer.h"

#include "json.hpp"

#include <vector>
#include <thread>
#include <atomic>
#include <iostream>
#include <climits>
#include <mutex>

using Json = nlohmann::json;

struct Params{
    int linkInd;
    int nodeInd;
    CAEN_DGTZ_SAMFrequency_t frequency;
    unsigned int recordLength;
    uint8_t triggerDelay;
    CAEN_DGTZ_IOLevel_t triggerLevel;
    unsigned int offsetADC;
    uint32_t maxEventTransfer;
    unsigned int pulseCount;
    unsigned int prehistorySep;
    float firstShot;
    bool waitRT;
    uint32_t serial;
    std::mutex* mutex;
};

union Timestamp{
    char bytes[8];
    unsigned long long int integer;
};

class Node {
private:
    bool armed = false;
    CAEN_DGTZ_BoardInfo_t boardInfo;
    
    static inline const float RESOLUTION = 0.6103516;
        

public:
    Node(Params params);
    ~Node();
    Json status();
    Params params;
    bool arm();
    bool ok = false;
    int handle = 0;

    constexpr const static std::pair<unsigned short, unsigned short> zeroInd[16] = {
        {100, 200},
        {10, 500},
        {10, 500},
        {10, 500},
        {10, 500},
        {10, 500},
        {10, 500},
        {10, 500},
        {10, 500},
        {10, 500},
        {10, 500},
        {10, 500},
        {10, 500},
        {10, 500},
        {10, 500},
        {10, 500}
    };
    constexpr const static std::pair<unsigned short, unsigned short> signalInd[16] = {
            {240, 400},
            {520, 670},
            {520, 670},
            {520, 670},
            {520, 670},
            {520, 670},
            {520, 670},
            {520, 670},
            {520, 670},
            {520, 670},
            {520, 670},
            {520, 670},
            {520, 670},
            {520, 670},
            {520, 670},
            {520, 670}
    };

    std::atomic<long long> timestamp = LLONG_MAX;
    static inline const unsigned short EVT_SIZE = 34832;
    static inline const unsigned short MAX_EVENTS = 16384;
    static inline const unsigned char MAX_CH = 16;
    static inline const unsigned short MAX_DEPTH = 1024;
    char* readoutBuffer = NULL;
    uint32_t readoutBufferSize = 0;
    
    std::array<std::array<std::array<unsigned short, Node::MAX_DEPTH>, Node::MAX_CH>, Node::MAX_EVENTS> result;
    std::array<std::array<unsigned short, Node::MAX_CH>, Node::MAX_EVENTS> zero;
    std::array<std::array<unsigned short, Node::MAX_CH>, Node::MAX_EVENTS> ph_el;
    std::array<unsigned long int, Node::MAX_EVENTS> times;
    std::atomic<unsigned short> evCount;
    void disarm();
    std::atomic<unsigned short> trigger = 0;

};

class Link {
private:
    static inline unsigned short totalLinks = 0;
    std::jthread thread;
    
public:
    std::atomic<bool> ok = false;

    std::atomic<bool> armed = false;
    std::atomic<bool> requestArm = false;
    std::atomic<bool> requestDisarm = false;
    std::atomic<bool> requestSave = false;

    Link(): linkInd{totalLinks}{
      totalLinks++;
    };
    unsigned int linkInd; //const?
    std::vector<Node*> nodes;
    std::vector<Params> params;
    ~Link();
    Json status();
    void arm();
    void disarm();
    void trigger(unsigned short count){
        if(this->armed){
            for(auto& node: this->nodes){
                node->trigger += count;
            }
        }
    };
    void init();
};


#endif //GTS_CORE_LINK_H
