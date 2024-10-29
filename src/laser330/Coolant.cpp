//
// Created by user on 01.10.2024.
//

#include "Coolant.h"
#include "iostream"

using namespace std::chrono_literals;

Json Coolant::handleRequest(Json& request){
    Json resp;
    if(request.contains("reqtype")){
        if(request.at("reqtype") == "status"){
            resp = this->status();
        }else{
            resp = {
                    {"ok", false},
                    {"err", "reqtype not found"}
            };
        }
    }else{
        resp = {
                {"ok", false},
                {"err", "request has no 'reqtype'"}
        };
    }
    return resp;
}

void Coolant::cfn(struct mg_connection *c, int ev, void *ev_data) {
    //MG_INFO(("CFN"));
    auto* th = static_cast<Coolant *>(c->fn_data);
    if (ev == MG_EV_OPEN) {
        //MG_INFO(("CLIENT has been initialized"));
    } else if (ev == MG_EV_CONNECT) {
        mg_send(c, Coolant::request, 12);
        //MG_INFO(("CLIENT sent data"));
        //MG_INFO(("CLIENT connected"));
    } else if (ev == MG_EV_READ) {
        //assosiate recv with queue

        struct mg_iobuf *r = &c->recv;
        //MG_INFO(("CLIENT got data: %.*s", r->len, r->buf));

        //Request req = th->queue.top();
        if(r->len != 13){
            MG_INFO(("BAD packet size"));
        }else{
            float temp = 0;
            char tmp[4];
            memcpy(tmp, r->buf + 10, 1);
            memcpy(tmp + 1, r->buf + 9, 1);
            memcpy(tmp + 2, r->buf + 12, 1);
            memcpy(tmp + 3, r->buf + 11, 1);
            memcpy(&temp, tmp, 4);
            //std::cout << temp << std::endl;

            th->states.emplace_back(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(),
                                    temp);
            if(th->states.size() > Coolant::MAX_HISTORY_SIZE){
                th->states.pop_front();
            }
        }

        r->len = 0;  // Tell Mongoose we've consumed data
        c->is_draining = 1;
    } else if (ev == MG_EV_CLOSE) {
        //MG_INFO(("CLIENT disconnected"));
        th->curr_c=nullptr;
    } else if (ev == MG_EV_ERROR) {
        MG_INFO(("CLIENT error: %s", (char *) ev_data));
        //MG_INFO(("CLIENT error: "));
    } else if(ev == MG_EV_POLL){
        //MG_INFO(("laser TCP poll, size: %d", th->states.size()));
        //timeouts
    }else if(ev == MG_EV_ACCEPT || ev == MG_EV_RESOLVE) {
        //TCP accepted
    }else if(ev != MG_EV_WRITE){
        std::cout << "Unhandled event by cooler: " << ev << " connection ID: " << c->id << ' ';
        for(int i = 0; i < 4; i++){
            std::cout << (int)(c->rem.ip[i]) << '.';
        }
        std::cout << (int)c->rem.port << std::endl;
    }
}

void Coolant::reconnectSocket(void *arg) {
    auto* th = (Coolant*)arg;
    if (th->curr_c == nullptr) {
        //MG_INFO(("reconnect"));
        th->curr_c = mg_connect(th->mgr, Coolant::address, Coolant::cfn, th);
        //MG_INFO(("CLIENT %s", th->curr_c ? "connecting" : "failed"));
    }
}

Coolant::~Coolant() {
    std::cout << "~Coolant" << std::endl;
    mg_timer_free(&this->mgr->timers, this->watchdog);
}

Json Coolant::status() {
    if(this->states.empty()){
        return Json({
                            {"ok", false},
                            {"err", "coolant status is unknown"}
                    });
    }
    Json resp;
    resp["ok"] = true;
    resp["hist"] = {};
    for(auto point: states){
        resp["hist"].push_back({
                                       {"time_f", point.timestamp_ms * 1e-3},
                                       {"temperature", point.temp}
        });
    }
    return resp;
}
