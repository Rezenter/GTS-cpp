//
// Created by user on 31.10.2024.
//

#include "Storage.h"
#include "Diagnostics.h"

#include <fstream>
#include <sstream>
#include <iomanip>


Json Storage::status() {
    if(!Storage::dbRoot.exists()){
        return Json({
                            {"ok", false},
                            {"err", "WTF? db root not found"},
                            {"also", Storage::dbRoot.path().string()}
                    });
    }

    if(!std::filesystem::directory_entry(dbRoot.path().string() + "debug\\").exists()){
        return Json({
                            {"ok", false},
                            {"err", "debugPath not found"},
                            {"also", std::filesystem::directory_entry(dbRoot.path().string() + "debug\\").path().string()}
                    });
    }

    if(!std::filesystem::directory_entry(dbRoot.path().string() + "plasma\\").exists()){
        return Json({
                            {"ok", false},
                            {"err", "plasmaPath not found"},
                            {"also", std::filesystem::directory_entry(dbRoot.path().string() + "plasma\\").path().string()}
                    });
    }

    if(!std::filesystem::directory_entry(dbRoot.path().string() + "debug\\SHOTN.txt").exists()){
        return Json({
                            {"ok", false},
                            {"err", "debugShotnPath not found"},
                            {"also", std::filesystem::directory_entry(dbRoot.path().string() + "debug\\SHOTN.txt").path().string()}
                    });
    }

    Json resp = {
            {"ok", true},
            {"shotn", Storage::shotn()},
            {"debug_shtn", Storage::shotn(false)},
            {"armed", this->armed}
    };
    if(this->armed){
        resp["path"] = this->currentPath;
        resp["isPlasma"] = this->isPlasma;
    }
    return resp;
}

void Storage::arm(){
    if(this->armed){
        std::cout << "attempt to arm already armed storage" << std::endl;
        return;
    }
    if(!Storage::status()["ok"]){
        std::cout << "attempt to arm bad storage" << std::endl;
        return;
    }

    this->isPlasma = this->diag->isPlasma;
    if(this->isPlasma){
        this->currentPath = std::filesystem::directory_entry(dbRoot.path().string() + "plasma\\");
    }else{
        this->currentPath = std::filesystem::directory_entry(dbRoot.path().string() + "debug\\");
    }

    std::string shotn = Storage::shotn(this->isPlasma);
    std::stringstream ss;
    ss << std::setw(5) << std::setfill('0') << shotn;

    std::filesystem::directory_entry candidate {this->currentPath.path().string() + "raw\\" + ss.str() + '\\'};
    this->count = 0;
    while(candidate.exists()){
        candidate = std::filesystem::directory_entry{this->currentPath.path().string() + "raw\\" + ss.str() + "_" + std::to_string(this->count) + '\\'};
        this->count++;
    }

    std::filesystem::create_directory(candidate);

    if(count == 0){
        std::filesystem::create_directory(std::filesystem::directory_entry{
                this->currentPath.path().string() + "slow\\raw\\" + ss.str() + '\\'});
    }else {
        std::filesystem::create_directory(std::filesystem::directory_entry{
                this->currentPath.path().string() + "slow\\raw\\" + ss.str() + "_" + std::to_string(this->count) + '\\'});
    }
    this->currentPath = candidate;
    std::cout << "Storage armed" << std::endl;
    this->armed = true;
}

void Storage::saveFast() {
    if(this->armed){
        Json header {
            {"config", this->config},
            {"boards", {}},
            {"version", 6}
        };


        int count = 0;
        std::stringstream filename;       
        std::ofstream outFile;
        { //block diag from deleting links.
            const std::lock_guard<std::mutex> lock(this->diag->caens.vectorMutex);
            for(auto& link: this->diag->caens.links){
                for(auto& node: link->nodes){
                    std::cout << "Save got events: " << node->evCount << std::endl;
                    Json boardData = Json::array();
                    for (size_t event_ind = 0; event_ind < node->evCount; event_ind++) {
                        boardData.push_back({
                                                    {"ch",    node->result[event_ind]},
                                                    {"ph_el", node->ph_el[event_ind]},
                                                    {"t",     (double)(node->times[event_ind] - node->times[0]) * 5e-6},
                                                    {"t_raw", node->times[event_ind]}
                                            });
                    }
                    //boardData[0]["t"] = 0;

                    filename.str(std::string());
                    filename << count++ << ".msgpk";
                    outFile.open(this->currentPath.path().string() + filename.str(), std::ios::out | std::ios::binary);
                    for (const auto &e : Json::to_msgpack(boardData)) outFile << e;
                    outFile.close();
                    header["boards"].push_back(Json({
                        {"ser", node->params.serial},
                        {"count", node->evCount.load()},
                    }));
                }
                link->requestSave = false;
            }
        }

        
        outFile.open(this->currentPath.path().string() + "header.json");
        outFile <<  header.dump(2) << std::endl;
        outFile.close();

        this->diag->savedFast = true;
        std::cout << "CAEN files written: " << this->currentPath.path().string() << std::endl;
    }else{
        std::cout << "attempt to save not armed storage" << std::endl;
        return;
    }
}

void Storage::saveOphir(unsigned short count) {
    if(this->armed){
        std::cout << "saving Ophir " << count << std::endl;
        std::ofstream outFile;

        //int count = 0;
        std::stringstream filename;

        Json data = Json::array();
        for (size_t event_ind = 0; event_ind < count; event_ind++) {
            data.push_back(Json::array({(this->diag->ophir.times[event_ind] - this->diag->ophir.times[0]), this->diag->ophir.energy[event_ind]}));
        }
        

        filename.str(std::string());
        filename << "ophir.msgpk";
        outFile.open(this->currentPath.path().string() + filename.str(), std::ios::out | std::ios::binary);
        for (const auto &e : Json::to_msgpack(data)) outFile << e;
        outFile.close();

        this->diag->savedOphir = true;
        std::cout << "Ophir file written: " << this->currentPath.path().string() << std::endl;
    }else{
        std::cout << "attempt to save not armed storage" << std::endl;
        return;
    }
}

void Storage::disarm(){
    if(this->armed){
        this->armed = false;
        if(!this->isPlasma){
            int shotn = std::stoi(Storage::shotn(false)) + 1;
            std::ofstream file;
            file.open(std::filesystem::directory_entry(dbRoot.path().string() + "debug\\SHOTN.txt"));
            //std::stringstream ss;
            file << std::setw(5) << std::setfill('0') << shotn;
            //std::string s = ss.str();
            //file.write(ss, 5);
            file.close();
        }
    }
}

std::string Storage::shotn(bool isPlasma) {
    char shot_line[5];
    std::ifstream file;
    if(isPlasma){
        if(!plasmaShotnPath.exists()){
            return Json({
                                {"ok", false},
                                {"err", "SHOTN.txt is not found"},
                                {"also", Storage::plasmaShotnPath.path().string()}
                        });
        }
        file.open(Storage::plasmaShotnPath);
    }else{
        std::filesystem::directory_entry path(dbRoot.path().string() + "debug\\SHOTN.txt");
        if(!path.exists()){
            return Json({
                                {"ok", false},
                                {"err", "SHOTN.txt is not found"},
                                {"also", path.path().string()}
                        });
        }
        file.open(path);
    }




    file.read(&shot_line[0], 5);
    file.close();
    return shot_line;
}

Json Storage::getConfigs(){
    std::filesystem::directory_entry path;
    Json resp({
            {"ok", true},
            {"configs", {}},
            {"spectral", {}},
            {"abs", {}}
    });
    path = std::filesystem::directory_entry(Storage::dbRoot.path().string() + "config_cpp\\");
    if(!path.exists()){
        return Json({
                    {"ok", false},
                    {"err", "directory not found: " + path.path().generic_string()}
            });
    }
    for (auto const& dir_entry : std::filesystem::directory_iterator{path}) {
        resp["configs"].push_back(dir_entry.path().filename().string());
    }
    std::sort(resp["configs"].begin(), resp["configs"].end(), std::greater<std::string>());

    path = std::filesystem::directory_entry(Storage::dbRoot.path().string() + "calibration\\expected\\");
    if(!path.exists()){
        return Json({
                    {"ok", false},
                    {"err", "directory not found: " + path.path().generic_string()}
            });
    }
    for (auto const& dir_entry : std::filesystem::directory_iterator{path}) {
        resp["spectral"].push_back(dir_entry.path().filename().string());
    }
    std::sort(resp["spectral"].begin(), resp["spectral"].end(), std::greater<std::string>());

    path = std::filesystem::directory_entry(Storage::dbRoot.path().string() + "calibration\\abs\\processed\\");
    if(!path.exists()){
        return Json({
                    {"ok", false},
                    {"err", "directory not found: " + path.path().generic_string()}
            });
    }
    for (auto const& dir_entry : std::filesystem::directory_iterator{path}) {
        resp["abs"].push_back(dir_entry.path().filename().string());
    }
    std::sort(resp["abs"].begin(), resp["abs"].end(), std::greater<std::string>());

    return resp;
}

bool Storage::setConfig(std::string filename){
    std::filesystem::directory_entry configFile {Storage::dbRoot.path().string() + "config_cpp\\" + filename};
    if(!configFile.exists()){
        std::cout << "WTF? file not found: " << configFile.path().string() << std::endl;
        return false;
    }

    std::ifstream file;
    file.open(configFile);
    Json candidate = Json::parse(file);
    file.close();
    
    if(this->check(candidate)){
        candidate["name"] = filename;
        this->config = candidate;

        this->dbRoot = std::filesystem::directory_entry(candidate["paths"]["db"]);
        this->plasmaShotnPath = std::filesystem::directory_entry(candidate["paths"]["plasmaShotn"]);
        

        //recheck abs & spectral
    }
    
    return true;
}

bool Storage::check(Json& candidate){
    if(candidate.contains("type")){
        if(candidate.at("type") != "configuration_cpp"){
            std::cout << "configuration file has wrong type: " << to_string(candidate.at("type")) << std::endl;
            return false;
        }
    }else{
        std::cout << "configuration file has no field 'type'" << std::endl;
        return false;
    }
    
    if(!candidate.contains("laser")){
        std::cout << "Bad laser config: laser" << std::endl;
        return false;
    }
    if(candidate["laser"].size() <= 0){
        std::cout << "Bad config: laser[0]" << std::endl;
        return false;
    }
    if(!candidate["laser"][0].contains("ophir")){
        std::cout << "Bad ophir config: ophir" << std::endl;
        return false;
    }
    if(!candidate["laser"][0]["ophir"].contains("ADCSerial")){
        std::cout << "Bad ophir config: ADCSerial" << std::endl;
        return false;
    }
    if(!candidate["laser"][0]["ophir"].contains("channel")){
        std::cout << "Bad ophir config: channel" << std::endl;
        return false;
    }
    if(!candidate["laser"][0]["ophir"].contains("headSerial")){
        std::cout << "Bad ophir config: headSerial" << std::endl;
        return false;
    }
    if(!candidate["laser"][0]["ophir"].contains("ip")){
        std::cout << "Bad ophir config: ip" << std::endl;
        return false;
    }
    if(!candidate["laser"][0]["ophir"].contains("diffuser")){
        std::cout << "Bad ophir config: diffuser" << std::endl;
        return false;
    }
    if(!candidate["laser"][0]["ophir"].contains("mMode")){
        std::cout << "Bad ophir config: mMode" << std::endl;
        return false;
    }
    if(!candidate["laser"][0]["ophir"].contains("pulseLength")){
        std::cout << "Bad ophir config: pulseLength" << std::endl;
        return false;
    }
    if(!candidate["laser"][0]["ophir"].contains("range")){
        std::cout << "Bad ophir config: range" << std::endl;
        return false;
    }
    if(!candidate["laser"][0]["ophir"].contains("wavelength")){
        std::cout << "Bad ophir config: wavelength" << std::endl;
        return false;
    }

    if(!candidate.contains("fast adc")){
        std::cout << "config has no 'fast adc'" << std::endl;
        return false;
    }
    if(!candidate["fast adc"].contains("first_shot")){
        std::cout << "config has no fast_adc::'first_shot'" << std::endl;
        return false;
    }
    if(!candidate["fast adc"].contains("frequency GHz")){
        std::cout << "config has no fast_adc::'frequency GHz'" << std::endl;
        return false;
    }
    if(!candidate["fast adc"].contains("maxEventTransfer")){
        std::cout << "config has no fast_adc::'maxEventTransfer'" << std::endl;
        return false;
    }
    if(!candidate["fast adc"].contains("links")){
        std::cout << "config has no fast_adc::'links'" << std::endl;
        return false;
    }

    if(!candidate.contains("laser")){
        std::cout << "config has no 'laser'" << std::endl;
        return false;
    }
    if(!candidate["laser"][0].contains("pulse_count")){
        std::cout << "config has no laser::[0]::'pulse_count'" << std::endl;
        return false;
    }
    for(const auto& sett_link: candidate["fast adc"]["links"]){
        for(const auto& sett_node: sett_link){
            if(!sett_node.contains("serial")){
                std::cout << "config has no fast_adc::links::node::'serial'" << std::endl;
                return false;
            }
            if(!sett_node.contains("prehistorySep")){
                std::cout << "config has no fast_adc::'prehistorySep'" << std::endl;
                return false;
            }
            if(!sett_node.contains("trigger delay")){
                std::cout << "config has no fast_adc::'trigger delay'" << std::endl;
                return false;
            }
            if(!sett_node.contains("vertical offset")){
                std::cout << "config has no fast_adc::'vertical offset'" << std::endl;
                return false;
            }
            std::cout << "Voltage range: [" << (long int)sett_node["vertical offset"] - 1250 << ", " << sett_node["vertical offset"] + 1250 << "] mV." << std::endl;
        
            if(!sett_node.contains("record depth")){
                std::cout << "config has no fast_adc::'record depth'" << std::endl;
                return false;
            }
            if(!sett_node.contains("trigIsNIM")){
                std::cout << "config has no fast_adc::links::node::'trigIsNIM'" << std::endl;
                return false;
            }
            if(!sett_node.contains("syncRT")){
                std::cout << "config has no fast_adc::'syncRT'" << std::endl;
                return false;
            }
        }
    }
    
    if(!candidate.contains("auto")){
        std::cout << "config has no 'auto'" << std::endl;
        return false;
    }
    if(!candidate["auto"].contains("diag")){
        std::cout << "config has no auto::diag'" << std::endl;
        return false;
    }
    if(!candidate["auto"].contains("fast")){
        std::cout << "config has no auto::fast'" << std::endl;
        return false;
    }
    if(!candidate["auto"].contains("slow")){
        std::cout << "config has no auto::slow'" << std::endl;
        return false;
    }
    if(!candidate["auto"].contains("ophir")){
        std::cout << "config has no auto::ophir'" << std::endl;
        return false;
    }
    if(!candidate["auto"].contains("lasOn")){
        std::cout << "config has no auto::lasOn'" << std::endl;
        return false;
    }
    if(!candidate["auto"].contains("lasOff")){
        std::cout << "config has no auto::lasOff'" << std::endl;
        return false;
    }

    if(!candidate.contains("paths")){
        std::cout << "config has no 'paths'" << std::endl;
        return false;
    }
    if(!candidate["paths"].contains("db")){
        std::cout << "config has no 'db'" << std::endl;
        return false;
    }
    std::filesystem::directory_entry testPath {candidate["paths"]["db"]};
    if(!testPath.exists()){
        std::cout << "directory not found: " + testPath.path().generic_string() << std::endl;
        return false;
    }
    if(!candidate["paths"].contains("plasmaShotn")){
        std::cout << "config has no 'plasmaShotn'" << std::endl;
        return false;
    }
    testPath = std::filesystem::directory_entry(candidate["paths"]["plasmaShotn"]);
    if(!testPath.exists()){
        std::cout << "directory not found: " + testPath.path().generic_string() << std::endl;
        return false;
    }

    return true;
}

bool Storage::setSpectral(std::string filename){
    return true;
    
    std::filesystem::directory_entry configFile {Storage::dbRoot.path().string() + "config_cpp\\" + filename};
    if(!configFile.exists()){
        std::cout << "WTF? file not found: " << configFile.path().string() << std::endl;
        return false;
    }

    std::ifstream file;
    file.open(configFile);
    Json candidate = Json::parse(file);
    file.close();
    
    if(this->check(candidate)){
        candidate["name"] = filename;
        this->config = candidate;

        this->dbRoot = std::filesystem::directory_entry(candidate["paths"]["db"]);
        this->plasmaShotnPath = std::filesystem::directory_entry(candidate["paths"]["plasmaShotn"]);
        

        //recheck abs & spectral
    }
    
    return true;
}

bool Storage::setAbs(std::string filename){
    return true;
    
    std::filesystem::directory_entry configFile {Storage::dbRoot.path().string() + "config_cpp\\" + filename};
    if(!configFile.exists()){
        std::cout << "WTF? file not found: " << configFile.path().string() << std::endl;
        return false;
    }

    std::ifstream file;
    file.open(configFile);
    Json candidate = Json::parse(file);
    file.close();
    
    if(this->check(candidate)){
        candidate["name"] = filename;
        this->config = candidate;

        this->dbRoot = std::filesystem::directory_entry(candidate["paths"]["db"]);
        this->plasmaShotnPath = std::filesystem::directory_entry(candidate["paths"]["plasmaShotn"]);
        

        //recheck abs & spectral
    }
    
    return true;
}