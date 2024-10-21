//
// Created by user on 24.09.2024.
//

#include "Diagnostics.h"
#include <chrono>

#include "json.hpp"


using Json = nlohmann::json;

Json Diagnostics::handleRequest(Json& payload){
    Json resp;
    if(payload.contains("subsystem")){
        if(payload.at("subsystem") == "mirror"){

            //resp = mirror.requestHandler(payload);
        }else if(payload.at("subsystem") == "diag"){
            resp = {
                    {"ok", true}
            };
        }else if(payload.at("subsystem") == "laser330"){
            resp = laser.handleRequest(payload);
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
    resp["unix"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    return resp;
}

void Diagnostics::fn(struct mg_connection *c, int ev, void *ev_data) {
    auto* th = static_cast<Diagnostics *>(c->fn_data);

    if (ev == MG_EV_READ) {
        if(c->recv.len == 0){
            MG_INFO(("BAD packet: size == 0"));
        }else if(c->recv.len > 1){
            MG_INFO(("ignore long packet"));
        }else{
            if (c->recv.buf[0] == 255){
                MG_INFO(("TOKAMAK START"));
                using std::operator""ms;
                std::this_thread::sleep_for(300ms);
                //th->laser.setState(req);
                th->laser.stop();
            }else if (c->recv.buf[0] == 127){
                //th->laser.setState(req);
                th->laser.start();
                using std::operator""ms;
                MG_INFO(("TOKAMAK START -10s"));
            }else{
                MG_INFO(("Unknown UDP packet", c->recv.buf[0]));
            }
        }
        c->recv.len = 0;
    }else if (ev != MG_EV_POLL){
        MG_INFO(("UDP:8888 unhandled event"));
    }
}