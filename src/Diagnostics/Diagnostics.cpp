//
// Created by user on 24.09.2024.
//

#include "Diagnostics.h"
#include <chrono>
#include <fstream>

using Json = nlohmann::json;

Json Diagnostics::handleRequest(Json& payload){
    Json resp;
    if(payload.contains("subsystem")){
        if(payload.at("subsystem") == "mirror"){
            resp = {
                    {"ok", false},
                    {"err", "requested subsystem not implemented"}
            };
            //resp = mirror.requestHandler(payload);
        }else if(payload.at("subsystem") == "diag"){
            if(payload.contains("reqtype")){
                if(payload.at("reqtype") == "get_configs"){
                    resp = this->getConfigs();
                }else if(payload.at("reqtype") == "load_config"){
                    if(payload.contains("filename")) {
                        resp = this->loadConfig(payload.at("filename"));
                    }else{
                        resp = {
                                {"ok", false},
                                {"err", "request has no 'filename'"}
                        };
                    }
                }else if(payload.at("reqtype") == "status"){
                       resp = this->status();
                }else if(payload.at("reqtype") == "arm"){
                    this->arm();
                    resp = this->status();
                }else if(payload.at("reqtype") == "disarm"){
                    this->disarm();
                    resp = this->status();
                }else if(payload.at("reqtype") == "trig"){
                    this->trig();
                    resp = this->status();
                }else if(payload.at("reqtype") == "auto"){
                    if(payload.contains("state")){
                        this->fullAuto = payload["state"];
                        resp = {
                            {"ok", true}
                        };
                    }else{
                        resp = {
                            {"ok", false},
                            {"err", "state not found"}
                        };
                    }   
                }else if(payload.at("reqtype") == "isPlasma"){
                    if(payload.contains("state")){
                        this->isPlasma = payload["state"];
                        resp = {
                            {"ok", true}
                        };
                    }else{
                        resp = {
                            {"ok", false},
                            {"err", "state not found"}
                        };
                    }   
                }else if(payload.at("reqtype") == "autoFast"){
                    if(payload.contains("state")){
                        this->fastAuto = payload["state"];
                        resp = {
                            {"ok", true}
                        };
                    }else{
                        resp = {
                            {"ok", false},
                            {"err", "state not found"}
                        };
                    }   
                }else if(payload.at("reqtype") == "autoLasOn"){
                    if(payload.contains("state")){
                        this->lasAutoOn = payload["state"];
                        resp = {
                            {"ok", true}
                        };
                    }else{
                        resp = {
                            {"ok", false},
                            {"err", "state not found"}
                        };
                    }   
                }else if(payload.at("reqtype") == "autoLasOff"){
                    if(payload.contains("state")){
                        this->lasAutoOff = payload["state"];
                        resp = {
                            {"ok", true}
                        };
                    }else{
                        resp = {
                            {"ok", false},
                            {"err", "state not found"}
                        };
                    }   
                }else{
                    resp = {
                            {"ok", false},
                            {"err", "reqtype not found"}
                    };
                }
            }else{
                resp = {
                        {"ok", false},
                        {"err", "request has no 'reqtype'"}
                };
            }
        }else if(payload.at("subsystem") == "laser330"){
            resp = this->laser.handleRequest(payload);
        }else if(payload.at("subsystem") == "coolant"){
            resp = this->coolant.handleRequest(payload);
        }else if(payload.at("subsystem") == "fast"){
            resp = this->caens.handleRequest(payload);
        }else if(payload.at("subsystem") == "ophir"){
            //resp = this->ophir.handleRequest(payload);
        }else{
            resp = {
                    {"ok", false},
                    {"err", "requested subsystem not found"}
            };
        }

    }else{
        resp = {
                {"ok", false},
                {"err", "request has no 'subsystem'"}
        };
    }
    /*
    resp["unix"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    */
    return resp;
}

void Diagnostics::handleUDPBroadcast(struct mg_connection *c, int ev, void *ev_data) {
    auto* th = static_cast<Diagnostics *>(c->fn_data);

    if (ev == MG_EV_READ) {
        if(c->recv.len == 0){
            MG_INFO(("BAD packet: size == 0"));
        }else if(c->recv.len > 1){
            MG_INFO(("ignore long packet"));
        }else{
            if (c->recv.buf[0] == 255){
                if(th->lasAutoOff){
                    using std::operator""ms;
                    std::this_thread::sleep_for(300ms); //guarantee wait for plasma
                    th->laser.stop();
                }
                
                if(th->fullAuto){
                    th->disarm();
                }

                MG_INFO(("TOKAMAK START"));
            }else if (c->recv.buf[0] == 127){
                if(th->fullAuto){
                    th->arm();
                }

                MG_INFO(("TOKAMAK START -12s"));
            }else{
                MG_INFO(("Unknown UDP packet", c->recv.buf[0]));
            }
        }
        c->recv.len = 0;
    }else if (ev != MG_EV_POLL){
        MG_INFO(("UDP:8888 unhandled event"));
    }
}

void Diagnostics::arm(){
    if(this->storage.armed){
        std::cout << "arm command ignored: storage not ready" << std::endl;
        return;
    }
    this->storage.arm();

    if(this->lasAutoOn){
        this->laser.start();
    }


    if(this->fastAuto){
        this->caens.arm();
    }
    this->savedFast = !this->fastAuto;

    if(this->ophirAuto){
        //this->ophir.arm();
    }
    this->savedOphir = !this->ophirAuto;

    //expecting save thread?
    this->saving = std::jthread([th=this](std::stop_token stoken){
        bool done = false;
        while(!stoken.stop_requested() && !done){

            done = true;
            done &= th->savedFast;
            done &= th->savedOphir;
            //savedOphir, savedelse...

            /*
            std::cout << "cont: " << done << std::endl;
            std::cout << "caens: " << th->savedFast << std::endl;
            std::cout << "Ophir: " << th->savedOphir << std::endl;
*/

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(100ms);
        }
        std::cout << "saving loop exit" << std::endl;
        if(done){
            std::cout << "all saved" << std::endl;
        }else{
            std::cout << "WARNING! saving deadline missed, some data is not saved" << std::endl;
            std::cout << "caens: " << th->savedFast << std::endl;
            std::cout << "Ophir: " << th->savedOphir << std::endl;
        }
        th->storage.disarm();
    });
}

void Diagnostics::disarm(){
   
    if(this->lasAutoOff){
        this->laser.stop();
    }
    if(this->fastAuto){
        this->caens.disarm();
    }

    //save laser status
    //save config

//collect data:
        //ophir
        //slow
        //status
        //config
        //datetime

    //is_plasma?
    //calculate folder
    //save data

    //wait for all expected saves


    const auto start = std::chrono::system_clock::now();
    while(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count() <= 5000){
        if(!this->storage.armed){
            std::cout << "saving loop exited before deadline" << std::endl;
            break;
        }

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
    }

    this->saving.request_stop();
    this->saving.join();
}

void Diagnostics::trig(){
    // trig laser or fastADC
    //trig slow

    this->disarm();
}

Json Diagnostics::getConfigs() {
    std::filesystem::directory_entry configPath {Storage::dbRoot.path().string() + "config_cpp\\"};
    if(!configPath.exists()){
        return Json({
                    {"ok", false},
                    {"err", "hardcoded directory not found: d:\\data\\db\\config_cpp\\"}
            });
    }
    Json resp({
                {"ok", true},
                {"files", {}}
        });

    for (auto const& dir_entry : std::filesystem::directory_iterator{configPath}) {
        resp["files"].push_back(dir_entry.path().filename().string());
        
        //std::cout << dir_entry.path() << '\n';
    }
    std::sort(resp["files"].begin(), resp["files"].end(), std::greater<std::string>());

    return resp;
}

Json Diagnostics::loadConfig(std::string filename) {
    std::filesystem::directory_entry configFile {Storage::dbRoot.path().string() + "config_cpp\\" + filename};
    if(!configFile.exists()){
        return Json({
                     {"ok", false},
                     {"err", "WTF? file not found"},
                     {"also", configFile.path().string()}
             });
    }

    std::ifstream file;
    file.open(configFile);
    Json candidate = Json::parse(file);
    file.close();

    MG_INFO(("WARNING! config file is not checked"));
    //check config file fields, critical for code execution
    if(candidate.contains("type")){
        if(candidate.at("type") != "configuration_cpp"){
            return {
                    {"ok", false},
                    {"err", "configuration file has wrong type: " + to_string(candidate.at("type"))}
            };
        }
    }else{
        return {
                {"ok", false},
                {"err", "configuration file has no field 'type'"}
        };
    }


    this->config = candidate;
    Json resp({
                      {"ok", true},
                      {"config", this->config}
              });
    
    /*
    std::jthread ophirThread = std::jthread([th=this](std::stop_token stoken){
        SetThreadAffinityMask(GetCurrentThread(), 1 << 9);
        Ophir ophir(th);
        ophir.connect();
    });
*/
    this->caens.initialising = true;
    this->caens.init();
    this->caens.initialising = false;
    
    resp["status"] = this->status();

    return resp;
}

void Diagnostics::die() {
    std::cout << "Killing diag" << std::endl;
    std::cout << "  Killing coolant" << std::endl;
    //this->coolant;
    std::cout << "  Killing caens" << std::endl;
    //this->caens;
    std::cout << "  Killing laser" << std::endl;
    //this->laser;
    std::cout << "diag dead" << std::endl;
}

Json Diagnostics::status() {
    //std::cout << "check time in last states!" << std::endl;
    Json resp = {
            {"ok", true},
            {"storage", this->storage.status()},
            {"laser", this->laser.status()},
            {"coolant", this->coolant.status()},
            {"caens", this->caens.status()},
            //{"ophir", this->ophir.status()},
            {"auto", {
                    {"isPlasma", this->isPlasma},
                    {"full", this->fullAuto},
                    {"fast", this->fastAuto},
                    {"lasOn", this->lasAutoOn},
                    {"lasOff", this->lasAutoOff}
                }
            }
    };
    return resp;
}

void Diagnostics::save() {
    //collect data:
        //ophir
        //slow
        //status
        //config

    //is_plasma?
    //calculate folder
    //save data
}
