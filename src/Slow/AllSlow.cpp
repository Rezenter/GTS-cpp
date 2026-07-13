//
// Created by user on 23.12.2025.
//

#include "Diagnostics/Diagnostics.h"
#include <iostream>

#include "Slow/AllSlow.h"

Json AllSlow::handleRequest(Json& req){
    if(req.contains("reqtype")){
        if(req.at("reqtype") == "status"){
            return this->status();
        }else if(req.at("reqtype") == "arm"){
            this->arm();
            return this->status();
        }else if(req.at("reqtype") == "disarm"){
            this->disarm();
            return this->status();
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
};

Json AllSlow::status(){
    Json resp = {
            {"ok", this->initialised.load()},
            {"boards", {}},
            {"armed", this->armed.load()},
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()}
    };
    this->vectorMutex.lock();
    unsigned short initCount = 0;
    for(auto& board: this->boards){
        Json boardStatus = board->status();
        resp["timestamp"] = max(resp["timestamp"], boardStatus["timestamp"]);

        resp["boards"].push_back(boardStatus);
        if(!boardStatus["ok"]){
            resp["ok"] = false;
            //resp["err"] = "Dead board ind.: " + std::to_string(link->linkInd);
        }
    }
    this->vectorMutex.unlock();
    return resp;
};

void AllSlow::arm(){
    if(this->initialised.load() && !this->armed.load()){
        this->requestArm = true;
    }
};

void AllSlow::disarm(){
    if(this->initialised.load() && this->armed.load()){
        std::this_thread::sleep_for(500ms);
        //std::cout << "Slow got " << this->count.load() << " before disarm" << std::endl;
        this->requestDisarm = true;
    }
    this->armed = false;
};

void AllSlow::init(){
    if(this->initialised){
        if(this->armed){
            this->disarm();
        }

        this->vectorMutex.lock();
        while(!this->boards.empty()){
            delete this->boards.back();
            this->boards.pop_back();
        }
        this->vectorMutex.unlock();

        std::cout << "Vector size =  " << this->boards.size() << std::endl;
    }

    {
        const std::lock_guard<std::mutex> lock(this->vectorMutex);
        unsigned char i = 0;
        for(const auto& sett_link: this->diag->storage.config["slow adc"]["boards"]) {
            this->boards.push_back(new Slow(this->diag, i++));
        }
    }//        this->vectorMutex.unlock(); due to guard destruction

    //check status

    this->initialised = true;
};
