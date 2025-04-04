//
// Created by user on 31.10.2024.
//

#ifndef GTS_CORE_STORAGE_H
#define GTS_CORE_STORAGE_H

#include "string"
#include <filesystem>
#include <vector>

#include "json.hpp"
#include "Caen/Link.h"


using Json = nlohmann::json;

class Diagnostics;

class Storage {
public:
    Storage(Diagnostics* parent): diag{parent}{};

    inline static std::filesystem::directory_entry dbRoot {R"(d:\data\db\)"};
    inline static std::filesystem::directory_entry plasmaShotnPath   {R"(c:\shared\SHOTN.txt)"};
    
    Json status();
    void saveFast();
    void saveOphir(unsigned short count);
    void arm();
    void disarm();
    static std::string shotn(bool isPlasma=true);
    bool armed = false;
    Json getConfigs();
    bool setConfig(std::string filename);
    bool setSpectral(std::string filename);
    bool setAbs(std::string filename);
    Json config;

private:
    std::filesystem::directory_entry currentPath;
    unsigned short count = 0;
    bool isPlasma = true;
    Diagnostics* diag;
    bool check(Json& candidate);
};


#endif //GTS_CORE_STORAGE_H
