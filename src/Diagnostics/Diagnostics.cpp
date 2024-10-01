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
};