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

    if(!Storage::debugPath.exists()){
        return Json({
                            {"ok", false},
                            {"err", "debugPath not found"},
                            {"also", Storage::debugPath.path().string()}
                    });
    }

    if(!Storage::plasmaPath.exists()){
        return Json({
                            {"ok", false},
                            {"err", "plasmaPath not found"},
                            {"also", Storage::plasmaPath.path().string()}
                    });
    }

    if(!Storage::debugShotnPath.exists()){
        return Json({
                            {"ok", false},
                            {"err", "debugShotnPath not found"},
                            {"also", Storage::debugShotnPath.path().string()}
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
        this->currentPath = Storage::plasmaPath;
    }else{
        this->currentPath = Storage::debugPath;
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
        std::ofstream outFile;
        outFile.open(this->currentPath.path().string() + "header.json");
        outFile <<  diag->config.dump(2) << std::endl;
        outFile.close();

        int count = 0;
        std::stringstream filename;
        for(auto& link: this->diag->caens.links){
            for(auto& node: link->nodes){
                //std::cout << "Save got events: " << node->evCount << std::endl;
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
            }
        }
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
            file.open(Storage::debugShotnPath);
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
        if(!debugShotnPath.exists()){
            return Json({
                                {"ok", false},
                                {"err", "SHOTN.txt is not found"},
                                {"also", Storage::debugShotnPath.path().string()}
                        });
        }
        file.open(Storage::debugShotnPath);
    }

    file.read(&shot_line[0], 5);
    file.close();
    return shot_line;
}
