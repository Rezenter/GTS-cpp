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
        
        this->initialised = false;
        while(!this->links.empty()){
            delete this->links.back();
            this->links.pop_back();
        }       

        std::cout << "Vector size =  " << this->links.size() << std::endl;
        std::cout << "ADD CLOSING OF RT SOCKET!" << std::endl;
        
    }
    if(!diag->config.contains("fast adc")){
        return {
                {"ok", false},
                {"err", "config has no 'fast adc'"}
        };
    }
    if(!diag->config["fast adc"].contains("first_shot")){
        return {
                {"ok", false},
                {"err", "config has no fast_adc::'first_shot'"}
        };
    }
    Link::params.firstShot = diag->config["fast adc"]["first_shot"];

    if(!diag->config["fast adc"].contains("prehistorySep")){
        return {
                {"ok", false},
                {"err", "config has no fast_adc::'prehistorySep'"}
        };
    }
    Link::params.prehistorySep = diag->config["fast adc"]["prehistorySep"];

    if(!diag->config["fast adc"].contains("frequency GHz")){
        return {
                {"ok", false},
                {"err", "config has no fast_adc::'frequency GHz'"}
        };
    }
    long settings_frequency_MHz = std::lround((float)diag->config["fast adc"]["frequency GHz"] * 1000);
    if(settings_frequency_MHz == 3200){
        Link::params.frequency = CAEN_DGTZ_SAM_3_2GHz;
    }else if(settings_frequency_MHz == 1600){
        Link::params.frequency = CAEN_DGTZ_SAM_1_6GHz;
    }else if(settings_frequency_MHz == 800){
        Link::params.frequency = CAEN_DGTZ_SAM_800MHz;
    }else if(settings_frequency_MHz == 400){
        Link::params.frequency = CAEN_DGTZ_SAM_400MHz;
    }else{
        return {
                {"ok", false},
                {"err", "bad CAEN frequency"}
        };
    }

    if(!diag->config["fast adc"].contains("trigger delay")){
        return {
                {"ok", false},
                {"err", "config has no fast_adc::'trigger delay'"}
        };
    }
    Link::params.triggerDelay = diag->config["fast adc"]["trigger delay"];

    if(!diag->config["fast adc"].contains("vertical offset")){
        return {
                {"ok", false},
                {"err", "config has no fast_adc::'vertical offset'"}
        };
    }
    Link::params.offsetADC = diag->config["fast adc"]["vertical offset"];
    std::cout << "Voltage range: [" << (long int)Link::params.offsetADC - 1250 << ", " << Link::params.offsetADC + 1250 << "] mV." << std::endl;

    if(!diag->config["fast adc"].contains("record depth")){
        return {
                {"ok", false},
                {"err", "config has no fast_adc::'record depth'"}
        };
    }
    Link::params.recordLength = diag->config["fast adc"]["record depth"];

    if(!diag->config["fast adc"].contains("maxEventTransfer")){
        return {
                {"ok", false},
                {"err", "config has no fast_adc::'maxEventTransfer'"}
        };
    }
    Link::params.maxEventTransfer = diag->config["fast adc"]["maxEventTransfer"];

    if(!diag->config["fast adc"].contains("links")){
        return {
                {"ok", false},
                {"err", "config has no fast_adc::'links'"}
        };
    }

    if(!diag->config.contains("laser")){
        return {
                {"ok", false},
                {"err", "config has no 'laser'"}
        };
    }
    if(!diag->config["laser"][0].contains("pulse_count")){
        return {
                {"ok", false},
                {"err", "config has no laser::[0]::'pulse_count'"}
        };
    }
    Link::params.pulseCount = diag->config["laser"][0]["pulse_count"];
    Link::params.linkInd = 0;
    unsigned int boards = 0;
    

    for(const auto& sett_link: diag->config["fast adc"]["links"]){
        Link::params.nodeInd = 0;
        this->links.push_back(new Link());
        for(const auto& sett_node: sett_link){
            if(Link::params.nodeInd >= 8){ //max nodes per link
                std::cout << "link: " << Link::params.linkInd << ", node: " << Link::params.nodeInd << "Too many nodes!" << std::endl;
                return {
                        {"ok", false},
                        {"err", "CAEN link has too many nodes"}
                };
            }
            if(boards >= 8){ //our system limit
                std::cout << "link: " << Link::params.linkInd << ", node: " << Link::params.nodeInd << "Too many boards!" << std::endl;
                return {
                        {"ok", false},
                        {"err", "config has too many CAEN boards"}
                };
            }

            if(!sett_node.contains("serial")){
                //disconnect
                std::cout << "WARNING! shut down half-initialised system!\n\n\n" << std::endl;
                return {
                        {"ok", false},
                        {"err", "config has no fast_adc::links::node::'serial'"}
                };
            }
            if(!sett_node.contains("trigIsNIM")){
                //disconnect
                std::cout << "WARNING! shut down half-initialised system!\n\n\n" << std::endl;
                return {
                        {"ok", false},
                        {"err", "config has no fast_adc::links::node::'trigIsNIM'"}
                };
            }
            if(sett_node["trigIsNIM"]){
                Link::params.triggerLevel = CAEN_DGTZ_IOLevel_NIM;
            }else{
                Link::params.triggerLevel = CAEN_DGTZ_IOLevel_TTL;
            }

            std::cout << "link: " << Link::params.linkInd << ", node: " << Link::params.nodeInd << ", connecting CAEN board " << to_string(sett_node["serial"]) << std::endl;
            
            links.back()->addNode();//link append node

            Link::params.nodeInd++;
            boards++;
        }
        Link::params.linkInd++;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    this->servaddr.sin_family = AF_INET;
    this->servaddr.sin_port = htons(8080);
    this->servaddr.sin_addr.s_addr = inet_addr("192.168.10.56"); //    !!!!!!!!!!!!!    use config!

    
/*
    std::cout << "debug arm & software trigger" << std::endl;
    this->diag->storage.arm(false);
    this->arm();
    std::cout << "all armed" << std::endl;
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(2s);

    std::cout << "trigg..." << std::endl;
    for(auto& link: this->links){
        link->trigger(Link::params.pulseCount + 1);
    }

    std::this_thread::sleep_for(2s);
    std::cout << "force stop" << std::endl;
    this->disarm();
*/
    this->initialised = true;

    return {
            {"ok", true}
    };
}

Json AllBoards::handleRequest(Json &req) {
    if(req.contains("reqtype")){
        if(req.at("reqtype") == "status"){
            return this->status();
        }else if(req.at("reqtype") == "arm"){
            if (req.contains("header")) {
                this->diag->config["default_calibrations"] = req["header"];
            } else {
                this->diag->config["default_calibrations"] = {};
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
            {"initialising", this->initialising},
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()},
            {"curr", this->current_ind.load()}
    };
    for(auto& link: this->links){
        Json linkStatus = link->status();
        if(link->nodes.size()){
            resp["timestamp"] = max(resp["timestamp"], linkStatus["timestamp"]);
        }
        resp["links"].push_back(linkStatus);
        if(!linkStatus["ok"]){
            resp["ok"] = false;
            resp["err"] = "Dead caen link ind.: " + std::to_string(link->linkInd);
        }
    }
    return resp;
}

void AllBoards::arm() {
    if(this->armed){
        std::cout << "CAENs arm command ignored: already armed" << std::endl;
        return;
    }
    
    
    for(auto& link: this->links){
        link->arm();
    }

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
                for(auto& node: link->nodes){
                    ready = min(ready, node->evCount.load());
                    //ready &= (node->evCount > current.load());
                }
                stopped &= !link->armed;
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
            }
        }
        armed = false;
        
        std::cout << "allBoards worker is stopping, waiting for saving data" << std::endl;
        
        diag->storage.saveFast();
        return;
    });
    
    this->armed = true;
    //std::cout << "all armed" << std::endl;
}

void AllBoards::trigger(size_t count) {
    if(this->armed){
        for(auto& link: this->links){
            link->trigger(count);
        }
    }else{
        std::cout << "CAENs trigger command ignored: not armed" << std::endl;
    }
}

AllBoards::~AllBoards() {
    if(this->armed){
        this->disarm();
    }
    
    while(!this->links.empty()){
        delete this->links.back();
        this->links.pop_back();
    }
}

void AllBoards::disarm() {
    if(!this->armed){
        std::cout << "CAENs disarm command ignored: already disarmed" << std::endl;
        return;
    }
    for(auto& link: this->links){
        link->disarm();
    }
    this->worker.request_stop();
    this->current_ind = 0;
    //this->armed = false; //should be set by worker
}
