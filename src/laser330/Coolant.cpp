//
// Created by user on 01.10.2024.
//

#include "Coolant.h"
#include "iostream"

using namespace std::chrono_literals;

Json Coolant::handleRequest(Json& request){
    Json resp;
    if(request.contains("reqtype")){
        //MG_INFO(("REQUEST___________________"));
        if(request.at("reqtype") == "connect"){
            resp = {
                    {"ok", true}
            };
            this->connect();
        }else if(request.at("reqtype") == "status"){
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

void Coolant::connect() {
    if(this->connected){
        return;
    }
    this->watchdog = mg_timer_add(this->mgr, 300, MG_TIMER_REPEAT | MG_TIMER_RUN_NOW, Coolant::reconnectSocket, this);
    //run();
    /*
    queue.emplace(0,
                        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() + 500,
                        "J0700 31\n",
                        ""
                        );
    */
}

void Coolant::cfn(struct mg_connection *c, int ev, void *ev_data) {
    //MG_INFO(("CFN"));
    auto* th = static_cast<Coolant *>(c->fn_data);
    if (ev == MG_EV_OPEN) {
        //MG_INFO(("CLIENT has been initialized"));
    } else if (ev == MG_EV_CONNECT) {
        th->connected = true;
        //th->run();
        th->associatedThread = std::thread([&](){
            th->run();
        });

        //MG_INFO(("CLIENT connected"));
    } else if (ev == MG_EV_READ) {
        //assosiate recv with queue

        struct mg_iobuf *r = &c->recv;
        //MG_INFO(("CLIENT got data: %.*s", r->len, r->buf));

        Request req = th->queue.top();
        if(r->len == 0){
            MG_INFO(("BAD packet: size == 0"));
        }else if(r->buf[r->len - 1] != '\n'){
            MG_INFO(("BAD packet: end != \\n"));
        }else{
            if (r->buf[0] == 'A'){
                if(r->len >= 5){
                    uint8_t code = std::stoul(std::string(reinterpret_cast<char *>(r->buf + 1)), nullptr, 16);
                    if(code == 0){
                        //ok
                    }else if(code == 1){
                        //Команда расшифрована, поставлена в очередь на выполнение.
                    }else if(code == 2){
                        MG_INFO(("Laser rejected command ", req.request.c_str()));
                    }else if(code == 3){
                        MG_INFO(("Laser rejected command, wrong parameters", req.request.c_str()));
                    }else if(code == 4){
                        MG_INFO(("Laser rejected command. Remote control is blocked. ", req.request.c_str()));
                    }else if(code == 5){
                        MG_INFO(("Laser rejected command. Unknown. ", req.request.c_str()));
                    }else if(code == 6){
                        MG_INFO(("Laser rejected command. Bad command.", req.request.c_str()));
                    }else if(code == 7){
                        MG_INFO(("Laser rejected command. Bad crc.", req.request.c_str()));
                    }else{
                        MG_INFO(("Laser rejected command, unknown code %s : %.*s", req.request.c_str(), r->len, r->buf));
                    }
                }
            }else if(r->buf[0] == 'K') {
                if(r->len != 13){
                    MG_INFO(("BAD packet: length %d", r->len));
                    MG_INFO((": %.*s", r->len, r->buf));
                }else {
                    uint8_t crc = 0;
                    for (size_t i = 0; i < r->len - 3; i++) {
                        crc += r->buf[i];
                        //MG_INFO(("CRC calc: %d", crc));
                    }
                    //MG_INFO(("BAD packet: length %d", r->len));
                    //MG_INFO(("CLIENT crc: %.*s", 2, r->buf + (r->len - 3)));

                    uint8_t crc_got = std::stoul(std::string(reinterpret_cast<char *>(r->buf + (r->len - 3))), nullptr, 16);
                    //MG_INFO(("CRC  got: %d", crc_got));
                    if(crc != crc_got){
                        MG_INFO(("BAD packet CRC"));
                    }else{
                        if(*(r->buf + 2) == '7'){
                            //status
                            uint16_t bits = std::stoul(std::string(reinterpret_cast<char *>(r->buf + 5)), nullptr, 16);
                            uint8_t state = 0;

                            if(1 == ((bits >> 0) & 1)){ //  контактор замкнут
                                if(1 == ((bits >> 1) & 1)){ // ЗГ и накачка работают
                                    if(1 == ((bits >> 6) & 1)){ // is desync?
                                        state = 2;
                                    }else{
                                        state = 4; //sync allowed, wait for 5V
                                    }
                                }else{
                                    state = 1;
                                }
                            }else{
                                state = 0;        //контактор выключен
                            }
                            long long int timeout_ms;
                            if(!th->states.empty()){
                                if(th->states.back().state != state) {
                                    if (th->states.back().state * state > 4) {
                                        th->lastTimestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                std::chrono::system_clock::now().time_since_epoch()).count();
                                    }
                                }
                                if(state <= 1){
                                    timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - th->lastTimestamp_ms;
                                }else{
                                    if(th->states.back().state <= 1 and state >= 2){
                                        timeout_ms = 10*1000 + th->lastTimestamp_ms - std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                                        if(timeout_ms < 0){
                                            state = 3; // warm-up finished
                                            timeout_ms += 60*1000;
                                            if(timeout_ms < 0){
                                                //set idle
                                            }
                                        }
                                    }else{
                                        timeout_ms = 70*1000 + th->lastTimestamp_ms - std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                                        if(timeout_ms < 0){
                                            //set idle
                                        }
                                    }
                                }
                            }
                            uint16_t delayMO = 0;
                            uint16_t delayAmp = 0;
                            if(!th->states.empty()){
                                delayMO = th->states.back().delayMO;
                                delayAmp = th->states.back().delayAmp;
                            }

                            th->states.emplace_back(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(),
                                                    timeout_ms,
                                                    bits,
                                                    delayMO,
                                                    delayAmp,
                                                    state);
                            if(th->states.size() > Coolant::MAX_HISTORY_SIZE){
                                th->states.pop_front();
                            }

                            //MG_INFO(("bits: %d", bits));
                        }else if(*(r->buf + 2) == '8'){
                            //errors
                            MG_INFO(("laser errors: %.*s", r->len, r->buf));
                            MG_INFO(("Errors are not handled"));
                        }else if(*(r->buf + 2) == '6'){
                            //Master output delay
                            if(!th->states.empty()){
                                th->states.back().delayMO = std::stoul(std::string(reinterpret_cast<char *>(r->buf + 5)), nullptr, 16);
                            }
                        }else if(*(r->buf + 2) == '5'){
                            //Amplifier delay
                            if(!th->states.empty()){
                                th->states.back().delayAmp = std::stoul(std::string(reinterpret_cast<char *>(r->buf + 5)), nullptr, 16);
                            }
                        }else{
                            MG_INFO(("CLIENT got data: %.*s", r->len, r->buf));
                            MG_INFO(("BAD packet: unknown code"));
                        }
                    }
                }
            }
        }

        //
        th->queue.pop();
        //req.responce = r->buf;

        r->len = 0;  // Tell Mongoose we've consumed data
    } else if (ev == MG_EV_CLOSE) {
        MG_INFO(("CLIENT disconnected"));
        th->connected = false;
        th->requestStop();
        th->associatedThread.join();
        // signal we are done
        th->curr_c=nullptr;

        while(!(th->queue.empty() or th->queue.top().priority < 255)){
            MG_INFO(("connection closed, drop fired requests"));
            Request r = th->queue.top();
            th->queue.pop();
            r.priority = 200;
            th->queue.push(r);
        }
    } else if (ev == MG_EV_ERROR) {
        MG_INFO(("CLIENT error: %s", (char *) ev_data));
        //MG_INFO(("CLIENT error: "));
    } else if(ev == MG_EV_POLL){
        //MG_INFO(("laser TCP poll, size: %d", th->states.size()));

        if(!th->connected){
            //MG_INFO(("Connection is not ready"));
            return;
        }
        if(th->queue.empty()){
            return;
        }
        if(th->queue.top().priority == 255){
            //MG_INFO(("TCP poll, waiting for response"));

            if(th->queue.top().bestBefore < std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()){
                MG_INFO(("TCP poll, outdated request. BAD!!! response will crash"));
                std::cout << th->queue.top().request << " queue: " << th->queue.size() << std::endl;
                //th->queue.pop();
            }
            return;
        }

        Request r = th->queue.top();
        th->queue.pop();
        r.priority = 255;
        th->queue.push(r);
        mg_send(c, r.request.data(), r.request.size());
        //MG_INFO(("CLIENT sent data"));

        //if poll, check queue
        //timeouts
    }else if(ev != MG_EV_WRITE){
        std::cout << "Unhandled event by laser: " << ev << " connection ID: " << c->id << ' ';
        for(int i = 0; i < 4; i++){
            std::cout << (int)(c->rem.ip[i]) << '.';
        }
        std::cout << (int)c->rem.port << std::endl;
    }
}

void Coolant::reconnectSocket(void *arg) {
    auto* th = (Coolant*)arg;
    if (th->curr_c == nullptr) {
        MG_INFO(("reconnect"));
        th->curr_c = mg_connect(th->mgr, th->address, Coolant::cfn, th);
        MG_INFO(("CLIENT %s", th->curr_c ? "connecting" : "failed"));
    }
}

bool Coolant::payload(){
    uint8_t count = 0;
    while(true){
        if(count == 20){
            queue.emplace(0,
                          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() + 1000,
                          "J0600 30\n"
            );
            count = 0;
        }else if(count == 10){
            queue.emplace(0,
                          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() + 1000,
                          "J0500 2F\n"
            );
            count++;
        }else{
            queue.emplace(1,
                          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() + 1000,
                          "J0700 31\n"
            );
            count++;
        }
        std::this_thread::sleep_for(500ms);
    }
    return false;
}

void Coolant::beforePayload() {
    Stoppable::beforePayload();
}

void Coolant::afterPayload() {
    Stoppable::afterPayload();
}

Coolant::~Coolant() {
    std::cout << "~Coolant" << std::endl;
    while(!this->queue.empty()){
        this->queue.pop();
    }
    std::cout << "queue cleaned" << std::endl;
    this->requestStop();
    this->associatedThread.join();
    std::cout << "all joined" << std::endl;

    //mg_timer_add(this->mgr, 300, MG_TIMER_REPEAT | MG_TIMER_RUN_NOW, Coolant::reconnectSocket, this);
    mg_timer_free(&this->mgr->timers, this->watchdog);
}

Json Coolant::status() {
    if(this->states.empty()){
        return Json({
                            {"ok", false},
                            {"err", "laser status is unknown"}
                    });
    }
    Json resp = R"(
        {
            "ok": true,
            "flags": [
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false
            ]
        }
    )"_json;
    State last = states.back();
    resp["timestamp_ms"] = last.timestamp_ms;
    for(size_t i = 0; i < 16; i++){
        resp["flags"][i] = (1 == ((last.bits >> i) & 1));
    }
    resp["state"] = last.state;
    resp["timeout"] = (double) last.timeout_ms * 1e-3;
    resp["delayMO"] = last.delayMO;
    resp["delayAmp"] = last.delayAmp;
    return resp;
}