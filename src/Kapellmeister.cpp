//
// Created by nz on 22.08.2023.
//
#include "Kapellmeister.h"
#include "iostream"
#include "bit"

void Kapellmeister::eventProcessorUDP(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    if (ev == MG_EV_READ) {
        MG_INFO(("Got a response"));

        /*
        for(int i = 0; i < c->recv.len; i++){
            //std::cout << (int)c->recv.buf[i] << std::endl;
        }
         */
        if(c->recv.len == sizeof(Reply::buffer)){
            memcpy(mirror.status.buffer, c->recv.buf, c->recv.len);
            if(mirror.status.status.calibration_1() == 6){
                mirror.span_1 = mirror.status.status.coordinate_1;
                MG_INFO(("Axis 1 calibrated"));
            }
            if(mirror.status.status.calibration_2() == 6){
                mirror.span_2 = mirror.status.status.coordinate_2;
                MG_INFO(("Axis 2 calibrated"));
            }
            //store timestamp
        }else{
            MG_INFO(("Bad response length"));
            std::cout << c->recv.len << ' ' << sizeof(Reply) << std::endl;
        }
        /*
        for(int i = 0; i < sizeof(c->rem.ip); i++){
            std::cout << (int)c->rem.ip[i] << std::endl;
            //192
            //168
            //10
            //70
            //0
            //0
            //0
            //0
            //0
            //...
        }
*/

        c->recv.len = 0UL;
    }
}

Json Kapellmeister::requestHandler(Json req){
    Json resp;

    if(req.contains("request")){
        int mirrorInd = 0;
        if(req.contains("mirror")){
            mirrorInd = (int)req.at("mirror") - 1;
            /*
            if(mirrorInd < 0 || mirrorInd > addresses.size()){
                return {
                        {"ok", false},
                        {"err", "mirror index out of bound"}
                };
            }
             */
        }else{
            return {
                    {"ok", false},
                    {"err", "mirror index is missing"}
            };
        }

        if(req.at("request") == "connect"){
            if(mirror.init){
                mirror.timer.stop();
                mg_mgr_free(&mirror.mgr);
                mirror.init = false;
            }
            mg_mgr_init(&mirror.mgr);
            mirror.c = mg_connect(&mirror.mgr, mirror.address.c_str(), eventProcessorUDP, NULL);
            mirror.init = true;

            mirror.timer.start(std::chrono::milliseconds(1000), [this, mirrorInd] {
                mirror.update();
            });
            mirror.update();

            resp = mirror.getStatus();
        }else
        if(req.at("request") == "disconnect"){
            if(mirror.init){
                mirror.timer.stop();
                mg_mgr_free(&mirror.mgr);
                mirror.init = false;
            }
            //close connections?
            resp = {
                    {"ok", true}
            };
        }else
        if(req.at("request") == "status"){
            resp = mirror.getStatus();
        }else
        if(req.at("request") == "move"){
            if(req.contains("axis")){
                std::string val = req.at("axis");
                uint8_t axis = 0;
                if(std::equal(val.begin(), val.end(), "vertical")){
                    axis = 0;
                }else if(std::equal(val.begin(), val.end(), "horizontal")){
                    axis = 1;
                }else{
                    return {
                            {"ok", false},
                            {"err", "bad axis"}
                    };
                }
                if(req.contains("speed")){
                    mirror.move(axis, (float)req.at("speed"));
                    return resp = {
                            {"ok", true}
                    };
                }if(req.contains("target")){
                    mirror.moveTo(axis, (int32_t )req.at("target"));
                    return resp = {
                            {"ok", true}
                    };
                }else{
                    return {
                            {"ok", false},
                            {"err", "movement must have speed or target"}
                    };
                }
            }else{
                return {
                        {"ok", false},
                        {"err", "mirror axis is missing"}
                };
            }
        }else
        if(req.at("request") == "stop"){
            mirror.stop();
            resp = {
                    {"ok", true}
            };
        }else if(req.at("request") == "calibrate"){
            mirror.calibrate();
            resp = {
                    {"ok", true}
            };
        }else{
            resp = {
                    {"ok", false},
                    {"err", "request is not listed"}
            };
        }
    }else{
        resp = {
                {"ok", false},
                {"err", "request is missing"}
        };
    }
    return resp;
}

void Mirror::update() {
    //check last connection time


    //if (c == NULL) return;
    this->outBuffer[0] = 0;
    this->outBuffer[1] = 0;
    this->outBuffer[2] = 0;
    this->outBuffer[3] = 0;
    this->outBuffer[4] = 0;
    this->outBuffer[5] = 0;
    this->outBuffer[6] = 0;
    this->outBuffer[7] = 0;
    this->outBuffer[8] = 0;
    this->outBuffer[9] = 0;

    mg_send(this->c, &outBuffer, sizeof outBuffer);

    mg_mgr_poll(&this->mgr, 500);
    //std::puts("received or timeout\n");
}

void Mirror::move(int axis, float speed) {
    this->outBuffer[0] = 0;
    this->outBuffer[1] = 0;
    this->outBuffer[2] = 0;
    this->outBuffer[3] = 0;
    this->outBuffer[4] = 0;
    this->outBuffer[5] = 0;
    this->outBuffer[6] = 0;
    this->outBuffer[7] = 0;
    this->outBuffer[8] = 0;
    this->outBuffer[9] = 0;

    this->outBuffer[axis * 5] = 0b01000000;
    std::memcpy(&outBuffer[axis * 5 + 1], &speed, sizeof(float));

    mg_send(this->c, &outBuffer, sizeof outBuffer);
    mg_mgr_poll(&this->mgr, 500);
}

void Mirror::moveTo(int axis, int32_t pos) {
    this->outBuffer[0] = 0;
    this->outBuffer[1] = 0;
    this->outBuffer[2] = 0;
    this->outBuffer[3] = 0;
    this->outBuffer[4] = 0;
    this->outBuffer[5] = 0;
    this->outBuffer[6] = 0;
    this->outBuffer[7] = 0;
    this->outBuffer[8] = 0;
    this->outBuffer[9] = 0;

    this->outBuffer[axis * 5] = 0b00100000;

    std::memcpy(&outBuffer[axis * 5 + 1], &pos, sizeof(int32_t));
    mg_send(this->c, &outBuffer, sizeof outBuffer);
    mg_mgr_poll(&this->mgr, 500);
}

void Mirror::stop() {
    this->outBuffer[0] = 0b10100000;
    this->outBuffer[1] = 0;
    this->outBuffer[2] = 0;
    this->outBuffer[3] = 0;
    this->outBuffer[4] = 0;
    this->outBuffer[5] = 0b10100000;
    this->outBuffer[6] = 0;
    this->outBuffer[7] = 0;
    this->outBuffer[8] = 0;
    this->outBuffer[9] = 0;

    mg_send(this->c, &outBuffer, sizeof outBuffer);
    mg_mgr_poll(&this->mgr, 500);
}

void Mirror::calibrate() {
    this->outBuffer[0] = 0b01100000;
    this->outBuffer[1] = 0;
    this->outBuffer[2] = 0;
    this->outBuffer[3] = 0;
    this->outBuffer[4] = 0;
    this->outBuffer[5] = 0b01100000;
    this->outBuffer[6] = 0;
    this->outBuffer[7] = 0;
    this->outBuffer[8] = 0;
    this->outBuffer[9] = 0;

    mg_send(this->c, &outBuffer, sizeof outBuffer);
    mg_mgr_poll(&this->mgr, 500);
}