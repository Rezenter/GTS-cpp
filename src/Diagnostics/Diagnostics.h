//
// Created by user on 24.09.2024.
//

#ifndef GTS_CORE_DIAGNOSTICS_H
#define GTS_CORE_DIAGNOSTICS_H

#include "mongoose.h"
#include "string"
#include <filesystem>

#include "json.hpp"
#include "laser330/Laser330.h"
#include "laser330/Coolant.h"
#include "Caen/AllBoards.h"

using Json = nlohmann::json;

//class AllBoards;

class Diagnostics {

private:
    mg_mgr* mgr;

    Coolant coolant;
    AllBoards caens;
    inline const static std::filesystem::directory_entry configPath {R"(d:\data\db\config_cpp\)"};
    static Json getConfigs();
    Json loadConfig(std::string filename);

public:
    Laser330 laser;
    Diagnostics(): caens(this), mgr{nullptr}{};
    static void fn(struct mg_connection *c, int ev, void *ev_data);
    void setMgr(mg_mgr *mgr){
        this->mgr = mgr;
        laser.setMgr(mgr);
        coolant.setMgr(mgr);
    };
    Json handleRequest(Json& payload);
    Json config;
};


#endif //GTS_CORE_DIAGNOSTICS_H
