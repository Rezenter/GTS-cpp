//
// Created by user on 24.09.2024.
//

#ifndef GTS_CORE_DIAGNOSTICS_H
#define GTS_CORE_DIAGNOSTICS_H

#include "string"
#include <filesystem>

#include "thread"
#include "json.hpp"
#include "laser330/Laser330.h"
#include "laser330/Coolant.h"
#include "Caen/AllBoards.h"
#include "Ophir/Ophir.h"
#include "Diagnostics/Storage.h"

using Json = nlohmann::json;

//class AllBoards;

class Diagnostics {

private:
    mg_mgr* mgr;

    Coolant coolant;
    
    //inline const static std::filesystem::directory_entry configPath {R"(d:\data\db\config_cpp\)"};
    static Json getConfigs();
    Json loadConfig(std::string filename);
    Json status();

    void trig();
    void arm();
    void disarm();
    
    bool fullAuto = false;
    bool fastAuto = false;
    bool lasAutoOn = false;
    bool lasAutoOff = true;
    bool ophirAuto = false;
    std::jthread saving;

public:
    bool isPlasma = true;
    Laser330 laser;
    Storage storage;
    AllBoards caens;
    Ophir ophir;

    Diagnostics(): caens(this), storage(this), mgr{nullptr}, ophir(this){};
    static void handleUDPBroadcast(struct mg_connection *c, int ev, void *ev_data);
    void setMgr(mg_mgr *mgr){
        this->mgr = mgr;
        laser.setMgr(mgr);
        coolant.setMgr(mgr);
    };
    Json handleRequest(Json& payload);
    Json config;
    void die();
    void save();

    bool savedFast = false;
    bool savedOphir = false;
};


#endif //GTS_CORE_DIAGNOSTICS_H
