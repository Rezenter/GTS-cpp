//
// Created by user on 24.09.2024.
//

#ifndef GTS_CORE_DIAGNOSTICS_H
#define GTS_CORE_DIAGNOSTICS_H

#include "string"

#include "thread"
#include "json.hpp"
#include "laser330/Laser330.h"
#include "laser330/Coolant.h"
#include "Caen/AllBoards.h"
#include "Ophir/Ophir.h"
#include "Diagnostics/Storage.h"
#include "Slow/AllSlow.h"

using Json = nlohmann::json;

//class AllBoards;

class Diagnostics {

private:
    Coolant coolant;
    
    //inline const static std::filesystem::directory_entry configPath {R"(d:\data\db\config_cpp\)"};
    static Json getConfigs();
    Json loadConfig(std::string filename, std::string spectral, std::string abs);


    void trig();
    void arm();
    void disarm();
    
    bool fullAuto = false;
    bool fastAuto = false;
    bool slowAuto = false;
    bool lasAutoOn = false;
    bool lasAutoOff = true;
    bool ophirAuto = false;
    std::jthread saving;

public:
    mg_mgr* mgr;
    bool isPlasma = true;
    Laser330 laser;
    Storage storage;
    AllBoards caens;
    Ophir ophir;
    //Slow slow;
    AllSlow allSlow;

    Diagnostics(): caens(this), storage(this), mgr{nullptr}, ophir(this), allSlow(this){};

    static void handleUDPBroadcast(struct mg_connection *c, int ev, void *ev_data);
    void setMgr(mg_mgr *mgr){
        this->mgr = mgr;
        laser.setMgr(mgr);
        coolant.setMgr(mgr);
    };
    Json handleRequest(Json& payload);
    Json status();
    void die();
    void save();

    bool savedFast = false;
    bool savedOphir = false;
    bool savedSlow = false;
};


#endif //GTS_CORE_DIAGNOSTICS_H
