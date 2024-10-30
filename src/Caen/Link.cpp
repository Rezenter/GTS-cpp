//
// Created by user on 24.10.2024.
//

#include "Link.h"

#include "iostream"

Node::Node(Params params) {
    this->evCount = 0;
    ok = CAEN_DGTZ_OpenDigitizer2(CAEN_DGTZ_OpticalLink, reinterpret_cast<void *>(&params.linkInd), params.nodeInd, 0, &this->handle) == CAEN_DGTZ_Success;
    if (!ok) {
        std::cout << "Can't open digitizer " << params.linkInd << ' ' <<  params.nodeInd << std::endl;
        return;
    }
    std::cout << "Connected digitizer " << params.linkInd << ' ' <<  params.nodeInd << std::endl;

    ok &= CAEN_DGTZ_GetInfo(this->handle, &boardInfo) == CAEN_DGTZ_Success;
    std::cout << "serial " << boardInfo.SerialNumber << std::endl;

    /* reset the digitizer */

    ok &= CAEN_DGTZ_Reset(handle) == CAEN_DGTZ_Success;
    if (!ok) {
        std::cout <<  "Unable to reset Node" << std::endl;
    }

    //Board Fail Status
    uint32_t d32 = 0;
    ok &= CAEN_DGTZ_ReadRegister(handle, 0x8178, &d32) == CAEN_DGTZ_Success;
    if ((d32 & 0xF) != 0) {
        std::cout <<  "Node Error: Internal Communication Timeout occurred." << std::endl;
        ok = false;
    }

    ok &= CAEN_DGTZ_SetGroupEnableMask(this->handle, 0b1111111111111111) == CAEN_DGTZ_Success;
    for (int sam_idx = 0; sam_idx < MAX_V1743_GROUP_SIZE; sam_idx++) {
        ok &= CAEN_DGTZ_SetSAMPostTriggerSize(this->handle, sam_idx, params.triggerDelay) == CAEN_DGTZ_Success;
    }
    ok &= CAEN_DGTZ_SetSAMSamplingFrequency(this->handle, params.frequency) == CAEN_DGTZ_Success;

    for (int channel = 0; channel < this->MAX_CH; channel++) {
        ok &= CAEN_DGTZ_DisableSAMPulseGen(handle, channel) == CAEN_DGTZ_Success;
    }

    ok &= CAEN_DGTZ_SetSWTriggerMode(handle, CAEN_DGTZ_TRGMODE_DISABLED) == CAEN_DGTZ_Success;
    ok &= CAEN_DGTZ_SetChannelSelfTrigger(handle, CAEN_DGTZ_TRGMODE_DISABLED, 0b1111111111111111) == CAEN_DGTZ_Success;
    ok &= CAEN_DGTZ_SetExtTriggerInputMode(handle, CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT) == CAEN_DGTZ_Success;

    for (int ch = 0; ch < 16; ch++) {
        ok &= CAEN_DGTZ_SetChannelDCOffset(this->handle, ch, params.offsetADC) == CAEN_DGTZ_Success;
    }

    /* Set Correction Level */
    //ret |= CAEN_DGTZ_SetSAMCorrectionLevel(handle, WDb->CorrectionLevel);
    std::cout << "what the fuck is SAM correction?\n\n" << std::endl;

    ok &= CAEN_DGTZ_SetMaxNumEventsBLT(this->handle, params.maxEventTransfer) == CAEN_DGTZ_Success;
    ok &= CAEN_DGTZ_SetRecordLength(this->handle, params.recordLength) == CAEN_DGTZ_Success;
    ok &= CAEN_DGTZ_SetIOLevel(this->handle, params.triggerLevel) == CAEN_DGTZ_Success;
    ok &= CAEN_DGTZ_SetAcquisitionMode(this->handle, CAEN_DGTZ_SW_CONTROLLED) == CAEN_DGTZ_Success;
    if (!ok) {
        std::cout << "ADC " << params.linkInd << ' ' <<  params.nodeInd << " initialisation error" << std::endl;
        return;
    }

    ok &= CAEN_DGTZ_MallocReadoutBuffer(this->handle, &this->readoutBuffer, &readoutBufferSize) == CAEN_DGTZ_Success;
    if (!ok) {
        std::cout << "ADC buffer allocation error" << std::endl;
        return;
    }
}

Node::~Node() {
    this->disarm();

    std::cout << "caen node destructor..." << std::endl;

    CAEN_DGTZ_FreeReadoutBuffer(&this->readoutBuffer);
    CAEN_DGTZ_CloseDigitizer(this->handle);

    std::cout << "caen node destructor OK" << std::endl;
}

Json Node::status() {
    //Board Fail Status
    ok = CAEN_DGTZ_GetInfo(this->handle, &this->boardInfo) == CAEN_DGTZ_Success;
    uint32_t d32 = 0;
    ok &= CAEN_DGTZ_ReadRegister(handle, 0x8178, &d32) == CAEN_DGTZ_Success;
    if ((d32 & 0xF) != 0) {
        std::cout <<  "Node Error: Internal Communication Timeout occurred." << std::endl;
        ok = false;
    }
    return {
            {"ok", ok},
            {"armed", this->armed}
    };
}

bool Node::arm() {
    if(this->armed){
        std::cout << "Node " << this->params.linkInd << '.' << this->params.nodeInd << " is already armed." << std::endl;
        return true;
    }

    this->evCount = 0;
    for(size_t evenInd = 0; evenInd < Node::MAX_EVENTS; evenInd++){
        times[evenInd] = 0;
        for(size_t ch_ind = 0; ch_ind < Node::MAX_CH; ch_ind++){
            zero[evenInd][ch_ind] = 0;
            ph_el[evenInd][ch_ind] = 0;
            for(size_t cell_ind = 0; cell_ind < Node::MAX_DEPTH; cell_ind++){
                result[evenInd][ch_ind][cell_ind] = 0;
            }
        }
    }

    this->armed = CAEN_DGTZ_GetInfo(this->handle, &this->boardInfo) == CAEN_DGTZ_Success;
    uint32_t d32 = 0;
    this->armed &= CAEN_DGTZ_ReadRegister(handle, 0x8178, &d32) == CAEN_DGTZ_Success;
    if ((d32 & 0xF) != 0) {
        std::cout <<  "Node Error: Internal Communication Timeout occurred." << std::endl;
        this->armed = false;
    }
    this->armed &= CAEN_DGTZ_ClearData(this->handle) == CAEN_DGTZ_Success;
    this->armed &= CAEN_DGTZ_SWStartAcquisition(this->handle) == CAEN_DGTZ_Success;
    return this->armed;
}

void Node::disarm() {
    if(this->armed){
        if (CAEN_DGTZ_SWStopAcquisition(this->handle) != CAEN_DGTZ_Success) {
            std::cout << "failed to stop ADC" << std::endl;
        }
        CAEN_DGTZ_ClearData(handle);
        this->armed = false;
    }
}

void Link::addNode() {
    this->nodes.push_back(new Node(Link::params));
}

Link::~Link() {
    std::cout << "caen link destructor..." << std::endl;
    this->disarm();
    while(!this->nodes.empty()){

        delete this->nodes.back();
        this->nodes.pop_back();
    }
    std::cout << "caen link destructor OK" << std::endl;
}

Json Link::status() {
    Json resp = {
            {"ok", true},
            {"nodes", {}}
    };
    for(auto& node: this->nodes){
        Json nodeStatus = node->status();
        resp["nodes"].push_back(nodeStatus);
        if(!nodeStatus["ok"]){
            resp["ok"] = false;
            resp["err"] = "Dead caen node ind.: " + std::to_string(node->params.nodeInd);
        }
    }
    return resp;
}

void Link::arm() {
    if(this->armed){
        std::cout << "Link " << this->linkInd << " is already armed." << std::endl;
        return;
    }

    this->armed = true;
    for(auto& node: this->nodes){
        this->armed &= node->arm();
    }
    if(!this->armed){
        std::cout << "link failed to arm" << std::endl;
        return;
    }

    this->worker = std::jthread([&nodes = this->nodes, &ind = this->linkInd](std::stop_token stoken){
        unsigned long long mask = 1 << ind; //allowed: 0b0000000011111111
        SetThreadAffinityMask(GetCurrentThread(), mask);
        std::cout << "Link thread: " << SetThreadAffinityMask(GetCurrentThread(), mask) << std::endl;

        while(!stoken.stop_requested()){
            bool allReady = true;
            for(auto& node: nodes){
                allReady &= node->pollNode() == (Link::params.pulseCount + 1);
                //allReady &= 0 == (Link::params.pulseCount + 1);
                std::cout << "events: " << node->evCount << std::endl;
            }
            if(allReady){
                std::cout << "link got all data" << std::endl;
                for(auto& node: nodes){
                    node->disarm();
                }
                return;
            }
        }
        std::cout << "link stopping due to request" << std::endl;
        for(auto& node: nodes){
            node->disarm();
        }
        return;
    });

}

void Link::disarm() {
    if(this->armed){
        this->worker.request_stop();
        this->worker.join();
        for(auto& node: this->nodes){
            node->disarm();
            std::cout << "Node " << params.linkInd << ' ' <<  params.nodeInd << " got " << node->evCount << std::endl;
        }
    }
}

//static members
Params Link::params;