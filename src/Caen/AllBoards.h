//
// Created by user on 24.10.2024.
//

#ifndef GTS_CORE_ALLBOARDS_H
#define GTS_CORE_ALLBOARDS_H

#include "Caen/Link.h"

#include "json.hpp"

#include <vector>
#include <thread>
#include <mutex>

using Json = nlohmann::json;

class Diagnostics;

struct Poly{
    uint32_t R;
    float Te;
    float ne;
    float Te_err;
    float ne_err;
};

struct LaserShot{
    unsigned short shotCount;
    unsigned char polyCount;
    Poly* poly;
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
    std::mutex mutex;
    std::mutex vectorMutex;
    void reinit();

private:
    std::jthread worker;
    Diagnostics* diag;
    void trigger(size_t count=1);
    bool armed = false;
    bool initialised = false;
    SOCKET sockfd;
    struct sockaddr_in servaddr;
    std::atomic<unsigned short> current_ind = 0;
    unsigned char boardCount = 0;
    bool emulate = false;

    char* shots;
    std::size_t eventSize;
    unsigned char polyCount;
};


#endif //GTS_CORE_ALLBOARDS_H
