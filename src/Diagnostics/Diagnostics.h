//
// Created by user on 24.09.2024.
//

#ifndef GTS_CORE_DIAGNOSTICS_H
#define GTS_CORE_DIAGNOSTICS_H

#include "mongoose.h"
#include "string"

#include "laser330/Laser330.h"
//#include "laser330/Coolant.h" //move to Laser

class Diagnostics {

private:
    mg_mgr* mgr;
    Laser330 laser;
public:
    static void fn(struct mg_connection *c, int ev, void *ev_data);
    void setMgr(mg_mgr *mgr){
        this->mgr = mgr;
        laser.setMgr(mgr);
    };
    Json handleRequest(Json& payload);
};


#endif //GTS_CORE_DIAGNOSTICS_H
