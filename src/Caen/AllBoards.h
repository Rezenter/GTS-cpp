//
// Created by user on 24.10.2024.
//

#ifndef GTS_CORE_ALLBOARDS_H
#define GTS_CORE_ALLBOARDS_H

#include "Caen/Link.h"

#include "json.hpp"

#include <vector>
#include <thread>

using Json = nlohmann::json;

class Diagnostics;


union Buffer{
    unsigned short int val[2];
    char chars[4];
};


class AllBoards {
public:
    explicit AllBoards(Diagnostics* parent): diag{parent} {};
    Json init();
    Json handleRequest(Json& req);
    ~AllBoards();
    Json status();
    std::vector<Link*> links;
    bool initialising = false;
    void arm();
    void disarm();

private:
    std::jthread worker;
    Diagnostics* diag;
    void trigger(size_t count=1);
    bool armed = false;
    bool initialised = false;
    SOCKET sockfd;
    struct sockaddr_in servaddr;
    Buffer buffer;
    std::atomic<unsigned short> current_ind = 0;
};


#endif //GTS_CORE_ALLBOARDS_H
