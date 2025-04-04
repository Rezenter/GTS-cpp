//
// Created by user on 24.10.2024.
//

#include "Caen/Link.h"

#include "iostream"

Node::Node(Params params) {
    this->evCount = 0;
    this->params = params;

    this->params.mutex->lock();
    this->ok = CAEN_DGTZ_OpenDigitizer2(CAEN_DGTZ_OpticalLink, reinterpret_cast<void *>(&this->params.linkInd), this->params.nodeInd, 0, &this->handle) == CAEN_DGTZ_Success;
    if (!ok) {
        std::cout << "Can't open digitizer " << this->params.linkInd << ' ' <<  this->params.nodeInd << std::endl;
        return;
    }
    this->params.mutex->unlock();
    

    this->ok &= CAEN_DGTZ_GetInfo(this->handle, &this->boardInfo) == CAEN_DGTZ_Success;
    if(this->params.serial != this->boardInfo.SerialNumber){
        std::cout << "bad serial " << this->params.linkInd << ' ' <<  this->params.nodeInd << " expected:" << this->params.serial << " got: " << this->boardInfo.SerialNumber << std::endl;
    }
    
    this->ok &= CAEN_DGTZ_Reset(this->handle) == CAEN_DGTZ_Success;
    if (!this->ok) {
        std::cout <<  "Unable to reset Node" << std::endl;
    }

    //Board Fail Status
    uint32_t d32 = 0;
    this->ok &= CAEN_DGTZ_ReadRegister(this->handle, 0x8178, &d32) == CAEN_DGTZ_Success;
    if ((d32 & 0xF) != 0) {
        std::cout <<  "Node Error: Internal Communication Timeout occurred." << std::endl;
        this->ok = false;
    }

    this->ok &= CAEN_DGTZ_SetGroupEnableMask(this->handle, 0b1111111111111111) == CAEN_DGTZ_Success;
    for (int sam_idx = 0; sam_idx < MAX_V1743_GROUP_SIZE; sam_idx++) {
        this->ok &= CAEN_DGTZ_SetSAMPostTriggerSize(this->handle, sam_idx, this->params.triggerDelay) == CAEN_DGTZ_Success;
    }
    this->ok &= CAEN_DGTZ_SetSAMSamplingFrequency(this->handle, this->params.frequency) == CAEN_DGTZ_Success;

    for (int channel = 0; channel < Node::MAX_CH; channel++) {
        this->ok &= CAEN_DGTZ_DisableSAMPulseGen(this->handle, channel) == CAEN_DGTZ_Success;
    }

    //ok &= CAEN_DGTZ_SetSWTriggerMode(handle, CAEN_DGTZ_TRGMODE_DISABLED) == CAEN_DGTZ_Success;
    this->ok &= CAEN_DGTZ_SetSWTriggerMode(this->handle, CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT) == CAEN_DGTZ_Success;
    this->ok &= CAEN_DGTZ_SetChannelSelfTrigger(this->handle, CAEN_DGTZ_TRGMODE_DISABLED, 0b1111111111111111) == CAEN_DGTZ_Success;
    this->ok &= CAEN_DGTZ_SetExtTriggerInputMode(this->handle, CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT) == CAEN_DGTZ_Success;

    for (int ch = 0; ch < 16; ch++) {
        this->ok &= CAEN_DGTZ_SetChannelDCOffset(this->handle, ch, this->params.offsetADC) == CAEN_DGTZ_Success;
    }

    /* Set Correction Level */
    //ret |= CAEN_DGTZ_SetSAMCorrectionLevel(handle, WDb->CorrectionLevel);
    //std::cout << "what the fuck is SAM correction?\n\n" << std::endl;

    this->ok &= CAEN_DGTZ_SetMaxNumEventsBLT(this->handle, this->params.maxEventTransfer) == CAEN_DGTZ_Success;
    this->ok &= CAEN_DGTZ_SetRecordLength(this->handle, this->params.recordLength) == CAEN_DGTZ_Success;
    this->ok &= CAEN_DGTZ_SetIOLevel(this->handle, this->params.triggerLevel) == CAEN_DGTZ_Success;
    this->ok &= CAEN_DGTZ_SetAcquisitionMode(this->handle, CAEN_DGTZ_SW_CONTROLLED) == CAEN_DGTZ_Success;
    if (!this->ok) {
        std::cout << "ADC " << this->params.linkInd << ' ' <<  this->params.nodeInd << " initialisation error" << std::endl;
        return;
    }

    if(this->params.pulseCount + 1 > Node::MAX_EVENTS){
        std::cout << "Error: experiment requeue " << this->params.pulseCount + 1 << " pulses. " << Node::MAX_EVENTS << " is maximum" << std::endl;
        return;
    }
    this->ok &= CAEN_DGTZ_MallocReadoutBuffer(this->handle, &this->readoutBuffer, &this->readoutBufferSize) == CAEN_DGTZ_Success;
    if (!this->ok) {
        std::cout << "ADC buffer allocation error" << std::endl;
        return;
    }
    this->timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

Node::~Node() {
    //std::cout << "caen node destructor... " << this->params.linkInd << std::endl;
    this->disarm();
    this->ok = false;
    this->params.mutex->lock();
    
    CAEN_DGTZ_FreeReadoutBuffer(&this->readoutBuffer);
    CAEN_DGTZ_CloseDigitizer(this->handle);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100us);

    this->params.mutex->unlock();
    //std::cout << "caen node destructed " << this->params.linkInd << std::endl;
}

Json Node::status() {
    return {
            {"ok", this->ok},
            {"armed", this->armed},
            {"timestamp", this->timestamp.load()}
    };
}

bool Node::arm() {
    if(this->armed){
        std::cout << "Node " << this->params.linkInd << '.' << this->params.nodeInd << " is already armed." << std::endl;
        return true;
    }

    this->trigger = 0;
    this->evCount = 0;
    for(unsigned short evenInd = 0; evenInd < Node::MAX_EVENTS; evenInd++){
        this->times[evenInd] = 0;
        for(unsigned char ch_ind = 0; ch_ind < Node::MAX_CH; ch_ind++){
            this->zero[evenInd][ch_ind] = 0;
            this->ph_el[evenInd][ch_ind] = 0;
            for(unsigned short cell_ind = 0; cell_ind < Node::MAX_DEPTH; cell_ind++){
                this->result[evenInd][ch_ind][cell_ind] = 0;
            }
        }
    }

    this->armed = CAEN_DGTZ_GetInfo(this->handle, &this->boardInfo) == CAEN_DGTZ_Success;
    uint32_t d32 = 0;
    this->armed &= CAEN_DGTZ_ReadRegister(this->handle, 0x8178, &d32) == CAEN_DGTZ_Success;
    if ((d32 & 0xF) != 0) {
        std::cout <<  "Node Error: Internal Communication Timeout occurred." << std::endl;
        this->armed = false;
    }
    CAEN_DGTZ_ReadData(this->handle,CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, this->readoutBuffer, &this->readoutBufferSize);
    this->armed &= CAEN_DGTZ_ClearData(this->handle) == CAEN_DGTZ_Success;
    this->armed &= CAEN_DGTZ_SWStartAcquisition(this->handle) == CAEN_DGTZ_Success;
    return this->armed;
}

void Node::disarm() {
    //std::cout << " node disarm" << std::endl;
    if(this->armed){
        this->trigger = 0;
        if (CAEN_DGTZ_SWStopAcquisition(this->handle) != CAEN_DGTZ_Success) {
            std::cout << "failed to stop ADC" << std::endl;
        }
        CAEN_DGTZ_ClearData(this->handle);
        this->armed = false;
    }
}

Link::~Link() {
    //std::cout << "caen link destructor..." << std::endl;

    while(this->requestSave.load()){
        std::cout << "waiting for save here" << std::endl;
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
    }
    
    this->disarm();
    this->ok = false;
    this->thread.request_stop();
    this->thread.join();
    //std::cout << "caen link destructor OK" << std::endl;
}

Json Link::status() {
    Json resp = {
            {"ok", this->ok.load()},
            {"nodes", {}},
            {"timestamp", 0}
    };
    long long timestamp = LLONG_MAX;
    for(auto& node: this->nodes){
        Json nodeStatus = node->status();
        
        timestamp = min(timestamp, (long long)nodeStatus["timestamp"]);
        resp["nodes"].push_back(nodeStatus);
        if(!nodeStatus["ok"]){
            resp["ok"] = false;
            resp["err"] = "Dead caen node ind.: " + std::to_string(node->params.nodeInd);
        }
    }
    resp["timestamp"] = timestamp;
    return resp;
}

void Link::arm() {
    if(this->armed || this->requestArm){
        std::cout << "Link " << this->linkInd << " is already armed." << std::endl;
        return;
    }
    this->requestArm = true;
}

void Link::disarm() {
    if(this->nodes.size() > 0){
        if(this->armed){
            this->requestDisarm = true;
            //this->thread.request_stop();
            //this->thread.join();
            this->armed = false;
        }
    }else{
        this->armed = false;
    }
}

void Link::init(){
    this->thread = std::jthread([th = this](std::stop_token stoken){
        unsigned long long mask = 1 << th->linkInd; //allowed: 0b0000000011111111
        SetThreadAffinityMask(GetCurrentThread(), mask);

        th->ok = true;
        for(const auto& param: th->params){
            //std::cout << "new CAEN " << param.linkInd << " " << param.nodeInd << std::endl;
            th->nodes.push_back(new Node(param));
            th->ok = th->ok.load() && th->nodes.back()->ok;
        }
        
        //check nodes in loop

        if(th->ok){
            //std::cout << "CAEN link init OK, entering command loop" << std::endl;
            
            bool allReady = false;
            const unsigned short target = (th->nodes.at(0)->params.pulseCount + 1);
            unsigned int short currentCell = 0;
            Timestamp timestampConverter;
            unsigned short event_ind = 0;
            while(!(stoken.stop_requested())){ 
                
                
                if(!th->armed.load() && th->requestArm.load()){  
                    th->requestDisarm = false;
                    th->armed = true;
                    for(auto& node: th->nodes){
                        if(!node->arm()){
                            th->armed = false;
                            break;
                        }
                    }
                    if(!th->armed){
                        std::cout << "link " << th->linkInd << " failed to arm" << std::endl;
                        break;
                    }
                    
                    th->requestArm = false;
                }
                if(th->armed.load()){
                    allReady = true;

                    //for(auto& node: th->nodes)
                    if(th->nodes.size() > 0){
                        auto& node = th->nodes.at(0);
                    
                        if(node->evCount.load() < target){
                            //std::cout << "link worker iteration " << th->linkInd << std::endl;            
                            if(CAEN_DGTZ_ReadData(node->handle, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, node->readoutBuffer, &node->readoutBufferSize) != CAEN_DGTZ_Success){
                                std::cout << "readout fuckup: "  << std::endl;
                                continue;
                            }
                            node->timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            
                            /*
                            if(node->readoutBufferSize % Node::EVT_SIZE != 0){
                                std::cout << "readoutBufferSize fuckup: "  << std::endl;
                                continue;
                            }*/

                            event_ind = 0;
                            while((event_ind + 1) * Node::EVT_SIZE <= node->readoutBufferSize){
                            //for(unsigned short event_ind = 0; event_ind < node->readoutBufferSize / Node::EVT_SIZE; event_ind++) {               
                                if(node->evCount >= node->MAX_EVENTS){
                                    std::cout << "Node poll ignored: already too many events in RAM"  << std::endl;
                                    break;
                                }
                    
                                char* group_pointer = node->readoutBuffer + Node::EVT_SIZE * event_ind + 16;
                    
                                timestampConverter.integer = 0;
                                timestampConverter.bytes[0] = *(group_pointer + 4 * 14 + 3);
                                timestampConverter.bytes[1] = *(group_pointer + 4 * 15 + 3);
                                timestampConverter.bytes[2] = *(group_pointer + 4 * 16 + 3);
                                timestampConverter.bytes[3] = *(group_pointer + 4 * 17 + 3);
                                timestampConverter.bytes[4] = *(group_pointer + 4 * 18 + 3);
                                node->times[node->evCount] = timestampConverter.integer;
                    
                                for(unsigned char groupIdx = 0; groupIdx < MAX_V1743_GROUP_SIZE; groupIdx++) {
                                    unsigned char ch1 = 2 * groupIdx;
                                    unsigned char ch2 = ch1 + 1;
                    
                                    for(unsigned char sector = 0; sector < 64; sector++) { // 64 sectors per 1024 cell page
                                        group_pointer += 4; // skip trash line, not mentioned in datasheet
                                        for(unsigned int cell = 0; cell < 16; cell++) {
                                            currentCell = sector * 16 + cell;
                                            node->result[node->evCount][ch1][currentCell] =
                                                    *reinterpret_cast<unsigned short *>((group_pointer + 4 * cell)) & 0x0FFF;
                                            node->result[node->evCount][ch1][currentCell] = (node->result[node->evCount][ch1][currentCell] & 0b011111111111) | (~node->result[node->evCount][ch1][currentCell] & 0b100000000000);
                                            
                                            if (node->zeroInd[ch1].first < currentCell && currentCell <= node->zeroInd[ch1].second) {
                                                node->zero[node->evCount][ch1] += node->result[node->evCount][ch1][currentCell];
                                            } else if (node->signalInd[ch1].first < currentCell && currentCell <= node->signalInd[ch1].second) {
                                                node->ph_el[node->evCount][ch1] += node->result[node->evCount][ch1][currentCell];
                                            }
                    


                                            node->result[node->evCount][ch2][sector * 16 + cell] =
                                                    *reinterpret_cast<unsigned short *>((group_pointer + 4 * cell + 1)) >> 4;
                                            node->result[node->evCount][ch2][currentCell] = (node->result[node->evCount][ch2][currentCell] & 0b011111111111) | (~node->result[node->evCount][ch2][currentCell] & 0b100000000000);
                                            
                                            if (node->zeroInd[ch2].first < currentCell && currentCell <= node->zeroInd[ch2].second) {
                                                node->zero[node->evCount][ch2] += node->result[node->evCount][ch2][currentCell];
                                            } else if (node->signalInd[ch2].first < currentCell && currentCell <= node->signalInd[ch2].second) {
                                                node->ph_el[node->evCount][ch2] += node->result[node->evCount][ch2][currentCell];
                                            }
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
                                    //calc Te, ne,
                                }
                                node->evCount++;
                                event_ind++;
                            }
                            
                            //using namespace std::chrono_literals;
                            //std::this_thread::sleep_for(200ms * th->linkInd);

                            if(node->trigger.load() != 0){
                                using namespace std::chrono_literals;
                                std::this_thread::sleep_for(2us);
                                if(CAEN_DGTZ_SendSWtrigger(node->handle) != CAEN_DGTZ_Success){
                                    std::cout << node->params.linkInd << " SW trigger failed" << std::endl;
                                }
                                node->trigger--;
                            }
                            
                            allReady &= false;
                        }
                        //std::cout << "events: " << node->evCount << std::endl;
                    }
                    if(allReady || th->requestDisarm.load()){
                        th->requestSave = true;
                        if(allReady){
                            std::cout << "link " << th->linkInd << " got all data" << std::endl;
                        }else{
                            std::cout << "link " << th->linkInd << " was preliminary stopped " << std::endl;
                        }
                        th->requestDisarm = false;
                        
                        for(auto& node: th->nodes){
                            node->disarm();
                        }
                        th->armed = false;
                        
                        using namespace std::chrono_literals;
                        std::this_thread::sleep_for(1s);  //allow gathering thread to do stuff
                        //call save
                        //break;
                    }
                }else{
                    for(auto& node: th->nodes){
                        uint32_t d32 = 999;
                        node->ok &= CAEN_DGTZ_ReadRegister(node->handle, 0x8178, &d32) == CAEN_DGTZ_Success;
                        if ((d32 & 0xF) != 0) {
                            std::cout <<  "Node Error: Internal Communication Timeout occurred." << std::endl;
                            node->ok = false;
                        }else{
                            node->timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                        }
                    }
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(100ms);  //not armed yet, save some CPU
                }
            } 
        }
        th->armed = false;
        th->ok = false;
        for(auto& node: th->nodes){
            node->disarm(); 
        }
        while(!th->nodes.empty()){
            delete th->nodes.back();
            th->nodes.pop_back();
        }
    });
}