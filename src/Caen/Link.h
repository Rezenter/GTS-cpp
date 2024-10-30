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
    bool ok = false;
    CAEN_DGTZ_BoardInfo_t boardInfo;
    int handle = 0;
    char* readoutBuffer = NULL;
    uint32_t readoutBufferSize = 0;
    bool armed = false;
    //static inline const size_t MAX_EVENTS = 262144;
    static inline const size_t MAX_EVENTS = 2621;
    static inline const size_t MAX_CH = 16;
    static inline const size_t MAX_DEPTH = 1024;
    static inline const size_t EVT_SIZE = 34832;
    unsigned int short currentCell = 0;
    static inline const float RESOLUTION = 0.6103516;
    constexpr const static std::pair<size_t, size_t> zeroInd[16] = {
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
    constexpr const static std::pair<size_t, size_t> signalInd[16] = {
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

public:
    Node(Params params);
    ~Node();
    Json status();
    Params params;
    bool arm();

    std::array<std::array<std::array<unsigned short, Node::MAX_DEPTH>, Node::MAX_CH>, Node::MAX_EVENTS> result;
    std::array<std::array<unsigned short, Node::MAX_CH>, Node::MAX_EVENTS> zero;
    std::array<std::array<unsigned short, Node::MAX_CH>, Node::MAX_EVENTS> ph_el;
    std::array<unsigned long int, Node::MAX_EVENTS> times;
    //std::array<std::latch*, SHOT_COUNT>& processed;
    std::atomic<size_t> evCount;
    void disarm();

    size_t pollNode(){
        if(CAEN_DGTZ_ReadData(this->handle,CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, readoutBuffer, &readoutBufferSize) != CAEN_DGTZ_Success){
            std::cout << "readout fuckup: "  << std::endl;
            return this->evCount;
        }
        if(readoutBufferSize % Node::EVT_SIZE != 0){
            std::cout << "readoutBufferSize fuckup: "  << std::endl;
            return this->evCount;
        }else {
            for(unsigned char event_ind = 0; event_ind < readoutBufferSize / Node::EVT_SIZE; event_ind++) {
                char* group_pointer = readoutBuffer + Node::EVT_SIZE * event_ind + 16;
                /*
                timestampConverter.integer = 0;
                timestampConverter.bytes[0] = *(group_pointer + 4 * 14 + 3);
                timestampConverter.bytes[1] = *(group_pointer + 4 * 15 + 3);
                timestampConverter.bytes[2] = *(group_pointer + 4 * 16 + 3);
                timestampConverter.bytes[3] = *(group_pointer + 4 * 17 + 3);
                timestampConverter.bytes[4] = *(group_pointer + 4 * 18 + 3);
                times[this->evCount] = timestampConverter.integer;
                 */
                std::memcpy(&times[this->evCount], group_pointer + 4 * 14 + 3, 5);

                for(int groupIdx = 0; groupIdx < MAX_V1743_GROUP_SIZE; groupIdx++) {
                    size_t ch1 = 2 * groupIdx;
                    size_t ch2 = ch1 + 1;

                    for(unsigned short sector = 0; sector < 64; sector++) { // 64 sectors per 1024 cell page
                        group_pointer += 4; // skip trash line, not mentioned in datasheet
                        for(unsigned int cell = 0; cell < 16; cell++) {
                            currentCell = sector * 16 + cell;
                            this->result[this->evCount][ch1][currentCell] =
                                    *reinterpret_cast<unsigned short *>((group_pointer + 4 * cell)) & 0x0FFF;
                            this->result[this->evCount][ch1][currentCell] = (result[this->evCount][ch1][currentCell] & 0b011111111111) | (~result[this->evCount][ch1][currentCell] & 0b100000000000);
                            /*
                            if (zeroInd[ch1].first < currentCell && currentCell <= zeroInd[ch1].second) {
                                zero[this->evCount][ch1] += result[this->evCount][ch1][currentCell];
                            } else if (signalInd[ch1].first < currentCell && currentCell <= signalInd[ch1].second) {
                                ph_el[this->evCount][ch1] += result[this->evCount][ch1][currentCell];
                            }
                             */


                            result[this->evCount][ch2][sector * 16 + cell] =
                                    *reinterpret_cast<unsigned short *>((group_pointer + 4 * cell + 1)) >> 4;
                            result[this->evCount][ch2][currentCell] = (result[this->evCount][ch2][currentCell] & 0b011111111111) | (~result[this->evCount][ch2][currentCell] & 0b100000000000);
                            /*
                            if (zeroInd[ch2].first < currentCell && currentCell <= zeroInd[ch2].second) {
                                zero[this->evCount][ch2] += result[this->evCount][ch2][currentCell];
                            } else if (signalInd[ch2].first < currentCell && currentCell <= signalInd[ch2].second) {
                                ph_el[this->evCount][ch2] += result[this->evCount][ch2][currentCell];
                            }
                             */
                        }
                        group_pointer += 4 * 16;
                    }
                    /*
                    zero[this->evCount][ch1] = (config->offset - 1250 + RESOLUTION * zero[this->evCount][ch1]) / (zeroInd[ch1].second - zeroInd[ch1].first);
                    zero[this->evCount][ch2] = (config->offset - 1250 + RESOLUTION * zero[this->evCount][ch2]) / (zeroInd[ch2].second - zeroInd[ch2].first);

                    ph_el[this->evCount][ch1] = config->offset - 1250 + RESOLUTION * ph_el[this->evCount][ch1];
                    unsigned int tmp = zero[this->evCount][ch1] * (signalInd[ch1].second - signalInd[ch1].first);
                    if (ph_el[this->evCount][ch1] > tmp){
                        ph_el[this->evCount][ch1] -= tmp;
                    }else{
                        ph_el[this->evCount][ch1] = 0;
                    }

                    ph_el[this->evCount][ch2] = config->offset - 1250 + RESOLUTION * ph_el[this->evCount][ch2];
                    tmp = zero[this->evCount][ch2] * (signalInd[ch2].second - signalInd[ch2].first);
                    if (ph_el[this->evCount][ch2] > tmp){
                        ph_el[this->evCount][ch2] -= tmp;
                    }else{
                        ph_el[this->evCount][ch2] = 0;
                    }
                     */
                }
                this->evCount++;
            }
        }
        return this->evCount;
    };

};

class Link {
private:
    static inline unsigned short totalLinks = 0;
    bool armed = false;
    std::jthread worker;

public:
    Link(): linkInd{totalLinks}{
      totalLinks++;
    };
    unsigned int linkInd;
    std::vector<Node*> nodes;
    void addNode();
    ~Link();
    Json status();
    void arm();
    static Params params;
    void disarm();
};


#endif //GTS_CORE_LINK_H
