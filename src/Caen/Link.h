//
// Created by user on 24.10.2024.
//

#ifndef GTS_CORE_LINK_H
#define GTS_CORE_LINK_H

#define CAEN_USE_DIGITIZERS
#define IGNORE_DPP_DEPRECATED

#include "include/CAENDigitizer.h"

#include "json.hpp"

#include "vector"

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
};

class Node {
private:
    CAEN_DGTZ_ErrorCode ret = CAEN_DGTZ_GenericError;
    CAEN_DGTZ_BoardInfo_t boardInfo;
    int handle = 0;
    char* readoutBuffer = NULL;
    uint32_t readoutBufferSize = 0;
    bool armed = false;

public:
    Node(Params params);
    ~Node();
    Json status();
    Params params;
    void arm();
};

class Link {
private:
    static inline unsigned short totalLinks = 0;
    bool armed = false;

public:
    Link(): linkInd{totalLinks}{
      totalLinks++;
    };
    unsigned int linkInd;
    std::vector<Node> nodes;
    void addNode();
    ~Link();
    Json status();
    void arm();
    static Params params;
};


#endif //GTS_CORE_LINK_H
