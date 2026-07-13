//
// Created by user on 06.11.2025.
//

#include "Slow.h"
#include "Diagnostics/Diagnostics.h"
#include <iostream>

Slow::~Slow(){
    std::cout << "Slow destructor" << std::endl;
    mg_timer_free(&this->diag->mgr->timers, this->watchdog);
    this->disarm();
    this->thread.request_stop();
    this->thread.join();
    std::cout << "Slow destructed" << std::endl;
};

void Slow::reconnectSocket(void *arg) {
    //MG_INFO(("reconnect"));
    auto* th = (Slow*)arg;
    if (th->curr_c == nullptr) {
        //MG_INFO(("reconnecting slow"));
        auto const test = "tcp://" + (std::string)(th->diag->storage.config["slow adc"]["boards"][th->num]["ip"]) + th->port;
        th->curr_c = mg_connect(th->diag->mgr, test.c_str(), Slow::cfn, th);
        //MG_INFO(("CLIENT %s", th->curr_c ? "connecting" : "failed"));
    }
}

void Slow::cfn(struct mg_connection *c, int ev, void *ev_data) {
    //MG_INFO(("CFN"));
    auto* th = static_cast<Slow *>(c->fn_data);
    if (ev == MG_EV_OPEN) {
        //MG_INFO(("CLIENT has been initialized"));
    } else if (ev == MG_EV_CONNECT) {
        mg_send(c, Slow::cmdStatus, 1);
        //MG_INFO(("CLIENT sent data"));
        //MG_INFO(("CLIENT connected"));
    } else if (ev == MG_EV_READ) {
        //assosiate recv with queue

        struct mg_iobuf *r = &c->recv;
        //MG_INFO(("CLIENT got data: %.*s", r->len, r->buf));

        //Request req = th->queue.top();
        if(r->len != 2){
            MG_INFO(("BAD packet size"));
        }else{
            unsigned short res1 = 0;
            unsigned short res2 = 0;
            memcpy(&res1, r->buf, 1);
            memcpy(&res2, r->buf + 1, 1);
            std::cout << r->len << ' ' << res1 << ' ' << res2 << std::endl;
            //std::cout << temp << std::endl;


            /*
             *  00 = not armed, idle
             *  4b = not armed, data ready
             */

            th->state = {
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(),
                    res2 == 0
            };
            //check armed
            //check isDataReady

        }

        r->len = 0;  // Tell Mongoose we've consumed data
        c->is_draining = 1;
    } else if (ev == MG_EV_CLOSE) {
        MG_INFO(("CLIENT disconnected"));
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
        std::cout << "Unhandled event by Slow: " << ev << " connection ID: " << c->id << ' ';
        for(int i = 0; i < 4; i++){
            std::cout << (int)(c->rem.ip[i]) << '.';
        }
        std::cout << (int)c->rem.port << std::endl;
    }
}


Json Slow::status(){
    return Json({
                        {"ok", this->state.ok},
                        {"init", this->init.load()},
                        {"armed", this->armed.load()},
                        {"dataReady", false},
                        {"timestamp", this->state.timestamp_ms}
                });
};

void Slow::arm(){
    if(this->init.load() && !this->armed.load()){
        this->requestArm = true;
    }
};

void Slow::disarm(){
    if(this->init.load() && this->armed.load()){
        std::this_thread::sleep_for(500ms);
        std::cout << "Slow got " << this->count.load() << " before disarm" << std::endl;
        this->requestDisarm = true;
    }
    this->armed = false;
};

void Slow::connect(){
    MG_INFO(("reconnect_fnc"));
    if(this->init.load()){
        MG_INFO(("disconnect"));
        this->disarm();
        this->thread.request_stop();
        this->thread.join();
    }

    this->thread = std::jthread([th=this](std::stop_token stoken){
        SetThreadAffinityMask(GetCurrentThread(), 1 << 10);

        //std::cout << th->diag->storage.config["slow adc"]["boards"][th->num]["ip"] << std::endl;

        if (th->curr_c == nullptr) {
            MG_INFO(("connecting slow"));
            //char *add = "tcp:/asd";
            //th->curr_c = mg_connect(th->mgr, th->address, Slow::cfn, th);
            th->watchdog = mg_timer_add(th->diag->mgr, 1000, MG_TIMER_REPEAT | MG_TIMER_RUN_NOW, Slow::reconnectSocket, th);

        }
        //th->init = true;

        if(th->init){
            MSG Msg;
            while(!(stoken.stop_requested())){
                if(!th->armed.load() && th->requestArm.load()){
                    //th->energy.fill(0);
                    //th->times.fill(0);

                    //arm

                    th->count = 0;
                    th->requestArm = false;
                    th->armed = true;
                }
                if(th->armed.load()){

                    std::this_thread::sleep_for(100ms);
                    th->timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

                    if(th->requestDisarm.load()){
                        //stop&save

                        std::cout << "request slow save: " << std::endl;
                        //std::cout << "request slow save: " << th->count.load() << " vs " << th->energy.size() << std::endl;
                        //th->diag->storage.saveOphir(min(th->count.load(), th->energy.size()));
                        th->armed = false;
                    }
                }else{
                    std::this_thread::sleep_for(100ms);

                    //check alive

                    th->timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                }
            }
        }
        std::cout << "Slow end" << std::endl;
        th->init = false;
        //close connections
    });
};
