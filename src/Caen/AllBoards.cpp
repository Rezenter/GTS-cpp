//
// Created by user on 24.10.2024.
//

#include "AllBoards.h"
#include "Diagnostics/Diagnostics.h"

#include "iostream"

Json AllBoards::init() {
    if(this->initialised){
        if(this->armed){
            this->disarm();
        }
        
        this->vectorMutex.lock();
        while(!this->links.empty()){
            delete this->links.back();
            this->links.pop_back();
        }
        this->vectorMutex.unlock();

        std::cout << "Vector size =  " << this->links.size() << std::endl;
        std::cout << "ADD CLOSING OF RT SOCKET!" << std::endl;
        
    }

    Params params;
    params.mutex = &this->mutex;

    //Link::params.firstShot = diag->config["fast adc"]["first_shot"];
    params.firstShot = diag->storage.config["fast adc"]["first_shot"];

    long settings_frequency_MHz = std::lround((float)diag->storage.config["fast adc"]["frequency GHz"] * 1000);
    if(settings_frequency_MHz == 3200){
        params.frequency = CAEN_DGTZ_SAM_3_2GHz;
    }else if(settings_frequency_MHz == 1600){
        params.frequency = CAEN_DGTZ_SAM_1_6GHz;
    }else if(settings_frequency_MHz == 800){
        params.frequency = CAEN_DGTZ_SAM_800MHz;
    }else if(settings_frequency_MHz == 400){
        params.frequency = CAEN_DGTZ_SAM_400MHz;
    }else{
        return {
                {"ok", false},
                {"err", "bad CAEN frequency"}
        };
    }

    params.maxEventTransfer = diag->storage.config["fast adc"]["maxEventTransfer"];

    params.pulseCount = diag->storage.config["laser"][0]["pulse_count"];
    params.linkInd = 0;
    this->boardCount = 0;
    
    {
        const std::lock_guard<std::mutex> lock(this->vectorMutex);
        for(const auto& sett_link: diag->storage.config["fast adc"]["links"]){
            params.nodeInd = 0;
            this->links.push_back(new Link());
            
            for(const auto& sett_node: sett_link){
                if(params.nodeInd >= 8){ //max nodes per link
                    std::cout << "link: " << params.linkInd << ", node: " << params.nodeInd << "Too many nodes!" << std::endl;
                    return {
                            {"ok", false},
                            {"err", "CAEN link has too many nodes"}
                    };
                }

                params.prehistorySep = sett_node["prehistorySep"];
                params.triggerDelay = sett_node["trigger delay"];
                params.offsetADC = sett_node["vertical offset"];
                params.recordLength = sett_node["record depth"];
                if(sett_node["trigIsNIM"]){
                    params.triggerLevel = CAEN_DGTZ_IOLevel_NIM;
                }else{
                    params.triggerLevel = CAEN_DGTZ_IOLevel_TTL;
                }
                params.waitRT = sett_node["syncRT"];
                params.serial = sett_node["serial"];

                std::cout << "link: " << params.linkInd << ", node: " << params.nodeInd << ", connecting CAEN board " << to_string(sett_node["serial"]) << std::endl;
                
                
                //links.back()->addNode();//link append node
                links.back()->params.emplace_back(params);

                params.nodeInd++;
                this->boardCount++;
            }
            params.linkInd++;

            //links.back()->init();
        }
    } //        this->vectorMutex.unlock(); due to guard destruction

/*
    memset(&servaddr, 0, sizeof(servaddr));
    this->servaddr.sin_family = AF_INET;
    this->servaddr.sin_port = htons(8080);
    this->servaddr.sin_addr.s_addr = inet_addr("192.168.10.56"); //    !!!!!!!!!!!!!    use config!
*/
    this->initialised = true;
    this->reinit();

    return {
            {"ok", true}
    };
}

void AllBoards::reinit(){
    this->vectorMutex.lock();
    if(this->initialised){
        for(auto& link: this->links){
            link->init();
        }
    }
    this->vectorMutex.unlock();
}

Json AllBoards::handleRequest(Json &req) {
    if(req.contains("reqtype")){
        if(req.at("reqtype") == "status"){
            return this->status();
        }else if(req.at("reqtype") == "arm"){
            if (req.contains("header")) {
                this->diag->storage.config["default_calibrations"] = req["header"];
            } else {
                this->diag->storage.config["default_calibrations"] = {};
            }
            this->arm();
            return this->status();
        }else if(req.at("reqtype") == "disarm"){
            this->disarm();
            return this->status();
        }else if(req.at("reqtype") == "trigger"){
            if(this->armed){
                size_t count = 1;
                if(req.contains("count")){
                    count = req.at("count");
                }

                this->trigger(count);
                
                Json resp = this->status();
                resp["triggered"] = true;
                return resp;
            }
            Json resp = this->status();
            resp["triggered"] = false;
            return resp;
        }else{
            return {
                    {"ok", false},
                    {"err", "reqtype not found"}
            };
        }
    }else{
        return {
                {"ok", false},
                {"err", "request has no 'reqtype'"}
        };
    }
}

Json AllBoards::status() {
    Json resp = {
            {"ok", this->initialised},
            {"links", {}},
            {"armed", this->armed},
            {"initialising", (int)this->initialising},
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()},
            {"curr", this->current_ind.load()}
    };
    this->vectorMutex.lock();
    unsigned short initCount = 0;
    for(auto& link: this->links){
        Json linkStatus = link->status();
        if(link->nodes.size()){
            resp["timestamp"] = max(resp["timestamp"], linkStatus["timestamp"]);
            for(auto& node: link->nodes){
                initCount += (unsigned short)node->ok;
            }
        }
        resp["links"].push_back(linkStatus);
        if(!linkStatus["ok"]){
            resp["ok"] = false;
            resp["err"] = "Dead caen link ind.: " + std::to_string(link->linkInd);
        }
    }
    this->vectorMutex.unlock();
    if(initCount != this->boardCount){
        resp["initialising"] = initCount;
    }
    return resp;
}

void AllBoards::arm() {
    if(this->armed){
        std::cout << "CAENs arm command ignored: already armed" << std::endl;
        return;
    }
    
    this->vectorMutex.lock();
    for(auto& link: this->links){
        link->arm();
    }
    this->vectorMutex.unlock();

    this->buffer.val[0] = 0;
    this->buffer.val[1] = 0;
    /*
    sendto(this->sockfd, this->buffer.chars, 4,
           0, (const struct sockaddr *) &this->servaddr,
           sizeof(this->servaddr));
*/
    this->current_ind = 0;

    this->worker = std::jthread([&links = this->links, &armed = this->armed, &diag = this->diag, &current = this->current_ind](std::stop_token stoken){
        //unsigned long long mask = 1 << 8; //allowed: 0b0001111100000000
        SetThreadAffinityMask(GetCurrentThread(), 1 << 8);
        //std::cout << "AllBoards thread: " << SetThreadAffinityMask(GetCurrentThread(), mask) << " " << GetCurrentThreadId() << std::endl;

        bool stopped = false;
        unsigned short ready = USHRT_MAX;
        while(!(stoken.stop_requested() || stopped)){
            ready = USHRT_MAX;
            stopped = true;

            for(auto& link: links){
                //std::cout << "check link " <<link->linkInd << std::endl;
                //for(auto& node: link->nodes){
                if(link->ok.load() && link->nodes.size() > 0){
                    auto& node = link->nodes[0];
                    if(node->params.waitRT){
                        ready = min(ready, node->evCount.load());
                    }
                }
                stopped &= !(link->armed.load() || link->requestArm.load());
            }
            
            while(ready > current.load()){
                // send UDP

                //check once
                //std::cout << "ready event " << current.load() << std::endl;
                
                /*
                sendto(sockfd, buffer.chars, 4,
                       0, (const struct sockaddr *) &servaddr,
                       sizeof(servaddr));
                */

                current++;

                if(current.load() == 1000){
                    std::cout << "RT got 1000" << std::endl;
                }
            }
        }
        armed = false;
        
        diag->storage.saveFast();
        current = 0;
        for(auto& link: links){
            if(link->ok.load() && link->nodes.size() > 0){
                auto& node = link->nodes[0];
                node->evCount = 0;
            }
        }
        return;
    });
    
    this->armed = true;
}

void AllBoards::trigger(size_t count) {
    if(this->armed){
        this->vectorMutex.lock();
        for(auto& link: this->links){
            link->trigger(count);
        }
        this->vectorMutex.unlock();
    }else{
        std::cout << "CAENs trigger command ignored: not armed" << std::endl;
    }
}

AllBoards::~AllBoards() {
    std::cout << "allBoards destructor" << std::endl;
    if(this->armed){
        this->disarm();
    }
    this->vectorMutex.lock();
    while(!this->links.empty()){
        delete this->links.back();
        this->links.pop_back();
    }
    this->vectorMutex.unlock();
    std::cout << "allBoards destructor OK" << std::endl;
}

void AllBoards::disarm() {
    if(!this->armed){
        std::cout << "CAENs disarm command ignored: already disarmed" << std::endl;
        return;
    }
    this->worker.request_stop();
    this->worker.join();
    this->vectorMutex.lock();
    for(auto& link: this->links){
        link->disarm();
    }
    this->vectorMutex.unlock();
    this->current_ind = 0;
    //this->armed = false; //should be set by worker
}
