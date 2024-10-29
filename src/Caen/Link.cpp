//
// Created by user on 24.10.2024.
//

#include "Link.h"

#include "iostream"

Node::Node(Params params) {
    //ret = CAEN_DGTZ_OpenDigitizer2(CAEN_DGTZ_OpticalLink, reinterpret_cast<void *>(&link), nodeIndex, 0, &handles[node]);
    ret = CAEN_DGTZ_OpenDigitizer2(CAEN_DGTZ_OpticalLink, reinterpret_cast<void *>(&params.linkInd), params.nodeInd, 0, &this->handle);
    if (ret != CAEN_DGTZ_Success) {
        std::cout << "Can't open digitizer " << params.linkInd << ' ' <<  params.nodeInd << std::endl;
        return;
    }
    std::cout << "Connected digitizer " << params.linkInd << ' ' <<  params.nodeInd << std::endl;

    ret = CAEN_DGTZ_GetInfo(this->handle, &boardInfo);
    std::cout << "serial " << boardInfo.SerialNumber << std::endl;

    ret = CAEN_DGTZ_SetSAMSamplingFrequency(this->handle, params.frequency);
    ret = CAEN_DGTZ_SetRecordLength(this->handle, params.recordLength);
    for (int sam_idx = 0; sam_idx < MAX_V1743_GROUP_SIZE; sam_idx++) {
        ret = CAEN_DGTZ_SetSAMPostTriggerSize(this->handle, sam_idx, params.triggerDelay);
    }

    ret = CAEN_DGTZ_SetGroupEnableMask(this->handle, 0b11111111);
    ret = CAEN_DGTZ_SetChannelSelfTrigger(this->handle, CAEN_DGTZ_TRGMODE_DISABLED, 0b11111111);
    //ret = CAEN_DGTZ_SetSWTriggerMode(this->handle,CAEN_DGTZ_TRGMODE_DISABLED);
    ret = CAEN_DGTZ_SetSWTriggerMode(this->handle, CAEN_DGTZ_TRGMODE_EXTOUT_ONLY);
    ret = CAEN_DGTZ_SetMaxNumEventsBLT(this->handle, params.maxEventTransfer);
    ret = CAEN_DGTZ_SetIOLevel(this->handle, params.triggerLevel);
    ret = CAEN_DGTZ_SetAcquisitionMode(this->handle, CAEN_DGTZ_SW_CONTROLLED);
    ret = CAEN_DGTZ_SetExtTriggerInputMode(this->handle, CAEN_DGTZ_TRGMODE_ACQ_ONLY);

    for (int ch = 0; ch < 16; ch++) {
        ret = CAEN_DGTZ_SetChannelDCOffset(this->handle, ch, params.offsetADC);
    }
    //ret = CAEN_DGTZ_SetChannelTriggerThreshold(this->handle, 0, config.triggerThresholdADC);

    if (ret != CAEN_DGTZ_Success) {
        std::cout << "ADC " << params.linkInd << ' ' <<  params.nodeInd << " initialisation error " << ret << std::endl;
        return;
    }

    ret = CAEN_DGTZ_MallocReadoutBuffer(this->handle, &this->readoutBuffer, &readoutBufferSize);
    if (ret != CAEN_DGTZ_Success) {
        std::cout << "ADC buffer allocation error " << ret << std::endl;
        return;
    }
}

Node::~Node() {
    //disarm();
    /*
    if(associatedThread.joinable()){
        associatedThread.join();
    }
     */
    std::cout << "caen node destructor..." << std::endl;

    CAEN_DGTZ_FreeReadoutBuffer(&this->readoutBuffer);
    CAEN_DGTZ_CloseDigitizer(this->handle);

    std::cout << "caen node destructor OK" << std::endl;
}

Json Node::status() {
    return {
            {"ok", CAEN_DGTZ_GetInfo(this->handle, &this->boardInfo) == CAEN_DGTZ_Success}
    };
}

void Node::arm() {
    if(this->armed){
        std::cout << "Node " << this->params.linkInd << '.' << this->params.nodeInd << " is already armed." << std::endl;
        return;
    }
    //pulses_count +1
    /*
    for(size_t evenInd = 0; evenInd < SHOT_COUNT; evenInd++){
        times[evenInd] = 0.0;
        for(size_t node = 0; node < 2; node++){
            for(size_t ch_ind = 0; ch_ind < CH_COUNT; ch_ind++){
                zero[node][evenInd][ch_ind] = 0.0;
                ph_el[node][evenInd][ch_ind] = 0.0;
                for(size_t cell_ind = 0; cell_ind < PAGE_LENGTH; cell_ind++){
                    result[node][evenInd][ch_ind][cell_ind] = 0.0;

                }
            }
        }
    }

    CAEN_DGTZ_ClearData(handles[node]);
    CAEN_DGTZ_SWStartAcquisition(handles[node]);


    associatedThread = std::thread([&](){
        run();
    });
     */
    this->armed = true;
}

void Link::addNode() {
    this->nodes.emplace_back(Link::params);
}

Link::~Link() {
    std::cout << "caen link destructor..." << std::endl;
    while(!this->nodes.empty()){
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
        Json nodeStatus = node.status();
        resp["nodes"].push_back(nodeStatus);
        if(!nodeStatus["ok"]){
            resp["ok"] = false;
            resp["err"] = "Dead caen node ind.: " + std::to_string(node.params.nodeInd);
        }
    }
    return resp;
}

void Link::arm() {
    if(this->armed){
        std::cout << "Link " << this->linkInd << " is already armed." << std::endl;
        return;
    }
    for(auto& node: this->nodes){
        node.arm();
    }
    this->armed = true;
}

//static members
Params Link::params;