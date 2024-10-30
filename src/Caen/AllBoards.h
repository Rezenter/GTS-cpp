//
// Created by user on 24.10.2024.
//

#ifndef GTS_CORE_ALLBOARDS_H
#define GTS_CORE_ALLBOARDS_H

//#include "Diagnostics/Diagnostics.h"
#include "Caen/Link.h"

#include "json.hpp"

#include "vector"

using Json = nlohmann::json;

class Diagnostics;

class AllBoards {
public:
    explicit AllBoards(Diagnostics* parent): diag{parent} {};
    Json init();
    Json handleRequest(Json& req);
    ~AllBoards();

private:
    std::vector<Link*> links;
    Diagnostics* diag;
    Json status();
    void arm();
    void disarm();

    bool armed = false;
};


#endif //GTS_CORE_ALLBOARDS_H
