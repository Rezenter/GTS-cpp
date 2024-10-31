//
// Created by user on 31.10.2024.
//

#include "Storage.h"

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

    return {
            {"ok", true},
            {"shotn", Storage::shotn()}
    };
}

void Storage::arm(bool isPlasma){
    if(this->armed){
        std::cout << "attempt to arm already armed storage" << std::endl;
        return;
    }
    if(!Storage::status()["ok"]){
        std::cout << "attempt to arm bad storage" << std::endl;
        return;
    }

    this->isPlasma = isPlasma;
    if(isPlasma){
        this->currentPath = Storage::plasmaPath;
    }else{
        this->currentPath = Storage::debugPath;
    }

    std::string shotn = Storage::shotn(isPlasma);
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
    std::cout << "Storage armed" << std::endl;
    this->armed = true;
}

void Storage::save(std::vector<Link*>* links) {
    if(this->armed){
        for(auto& link: *links){
            std::cout << "Save got events: " << link->nodes.back()->evCount << std::endl;
        }
        //if(){        }

        //is plasma
        //arm?
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
    }else{
        std::cout << "attempt to save not armed storage" << std::endl;
        return;
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
