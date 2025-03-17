//
// Created by user on 31.10.2024.
//

#ifndef GTS_CORE_STORAGE_H
#define GTS_CORE_STORAGE_H

#include "string"
#include <filesystem>
#include <vector>

#include "json.hpp"
#include "Diagnostics/Storage.h"
#include "Caen/Link.h"


using Json = nlohmann::json;

class Diagnostics;

class Storage {
public:
    Storage(Diagnostics* parent): diag{parent}{};

    inline const static std::filesystem::directory_entry dbRoot {R"(d:\data\db\)"};
    Json status();
    void save();
    void arm(bool isPlasma=true);
    static std::string shotn(bool isPlasma=true);
    bool armed = false;

private:
    //inline const static std::filesystem::directory_entry plasmaShotnPath   {R"(z:\SHOTN.txt)"};
    //inline const static std::filesystem::directory_entry plasmaShotnPath   {R"(\\172.16.12.28\Data\SHOTN.txt)"};
    inline const static std::filesystem::directory_entry plasmaShotnPath   {R"(\\172.16.12.70\shared\SHOTN.txt)"};
    inline const static std::filesystem::directory_entry debugPath    {dbRoot.path().string() + "debug\\"};
    inline const static std::filesystem::directory_entry plasmaPath    {dbRoot.path().string() + "plasma\\"};

    inline const static std::filesystem::directory_entry debugShotnPath    {debugPath.path().string() + "SHOTN.txt"};
    //inline const static std::filesystem::directory_entry gasPath           {R"(d:/data/db/gas/)"};
    //inline const static std::filesystem::directory_entry spectralPath      {R"(d:/data/db/calibration/expected/)"};
    //inline const static std::filesystem::directory_entry absPath           {R"(d:/data/db/calibration/abs/processed/)"};

    std::filesystem::directory_entry currentPath;
    unsigned short count = 0;
    bool isPlasma = true;
    Diagnostics* diag;
};


#endif //GTS_CORE_STORAGE_H
