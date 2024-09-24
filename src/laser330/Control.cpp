//
// Created by nz on 30.04.2020.
//

#include "Control.h"

Control::Control(Chatter &chatter) {
    this->chatter = &chatter;
    std::function<void(void)> func = std::bind(&Control::cyclic, this);
    std::thread([this, func]() {
        while (!exit){
            func();
            std::this_thread::sleep_for(std::chrono::milliseconds(cyclicTimeout));
        }
        printf("Cycle exit.\n");
    }).detach();
}

void Control::cyclic() {
    if(remoteRequired != remote){
        if(remote && stateCurrent != State::idle){
            stateRequired = State::idle;
            if(!stateRequest()){
                std::cout << "Failed to set idle state!" << std::endl;
            }
            stateCurrent = stateRequired;
            return;
        }
        if(!remoteRequest()){
            std::cout << "Failed to set control!" << std::endl;
        }
        remote = remoteRequired;
        return;
    }
    if(stateRequired != stateCurrent){
        if(stateRequired < stateCurrent || !timeLock){
            if(!stateRequest()){
                std::cout << "Failed to set state!" << std::endl;
            }
            stateCurrent = stateRequired;
            return;
        }
    }
    updateState();
}

void Control::updateState() {
    //std::cout << "update requested" <<std::endl;
    if(chatter->sendCmd(Chatter::cmd::status)){
        //std::cout << "asking status" << std::endl;
        Responce resp = chatter->recvResp();
        //std::cout << "responce" << std::endl;
        if(resp.success){
            currentStatus = resp.payload;
            //std::cout << currentStatus << std::endl;
            if((currentStatus >> Status::powerBit) & 1){
                remote = (currentStatus >> Status::remoteBit) & 1;
                if((currentStatus >> Status::generatorBit) & 1){
                    if((currentStatus >> Status::desyncBit) & 1){
                        if(timeLock){
                            stateCurrent = State::warmUp;
                        }else{
                            stateCurrent = State::readyToShoot;
                        }
                    }else {
                        stateCurrent = State::shooting;
                    }
                }else{
                    stateCurrent = State::idle;
                }
            }else{
                stateCurrent = State::error;
            }
            connected = true;
            displayState();
            return;
        }else{
            printf("Update Failed. No connection to laser!\n");
        }
    }else{
        printf("Update Failed. No connection to MOXA!\n");
    }
    connected = false;
}

void Control::displayState() {
    std::cout << animation[anim_counter] << " ";
    if(++anim_counter == 4){
        anim_counter = 0;
    }
    std::time_t today_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char mbstr[100];
    if (std::strftime(mbstr, sizeof(mbstr), "%Y.%m.%0d %H:%M:%S\t", std::localtime(&today_time))) {
        std::cout << mbstr;
    }

    switch (stateCurrent) {
        case State::idle:
            std::cout << "\t\033[1;32mIdle\033[m\t\t";
            break;
        case warmUp:
            printf("\t\033[1;33mWarming up... %.1fs\033[m\t\t",
                    std::chrono::duration_cast<std::chrono::milliseconds>(timeout -
                    std::chrono::system_clock::now().time_since_epoch()).count() * 0.001);
            break;
        case readyToShoot:
            std::cout << "\t\033[1;32mReady to shoot\033[m\t\t";
            //timeout
            break;
        case shooting:
            std::cout << "\t\033[1;31mShooting!\033[m\t\t";
            //timeout
            break;
        case coolDown:
            std::cout << "\t\033[1;33mCooling down...\033[m\t\t";
            //timeout
        case error:
            std::cout << "\t\033[1;31mError!\033[m\t\t";
            break;
    }

    if(remote){
        std::cout << "\033[1;32mRemote\033[m\t";
    }else{
        std::cout << "\033[1;31mLocal\033[m\t";
    }
    if((currentStatus >> Status::powerBit) & 1){
        std::cout << "\033[1;32mPower\033[m\t";
    }else{
        std::cout << "\033[1;31mPower\033[m\t";
    }
    if((currentStatus >> Status::generatorBit) & 1){
        std::cout << "\033[1;32mGen\033[m\t";
    }else{
        std::cout << "\033[1;31mGen\033[m\t";
    }
    if(((currentStatus >> Status::generatorBit) & 1) && !((currentStatus >> Status::desyncBit) & 1)){
        std::cout << "\033[1;33mBEAM\033[m\t";
    }else{
        std::cout << "\033[1;32mNO BEAM\033[m\t";
    }
    if((currentStatus >> Status::tempOKBit) & 1){
        std::cout << "\033[1;32mTemp\033[m\t";
    }else{
        std::cout << "\033[1;31mTemp\033[m\t";
    }

    for(int i = 0; i < 16; i++){
        std::cout << ((currentStatus >> i) & 1);
    }

    std::cout << "\r" << std::flush;
}

void Control::setState(Control::State newState) {
    if(!remote){
        std::cout << "\033[1;33mRemote control disabled.\033[m\n";
        //return;
    }
    switch (newState) {
        case State::idle:
            stateRequired = newState;
            break;
        case warmUp:
            std::cout << "\033[1;31mForbidden state to set.\033[m\n";
            break;
        case readyToShoot:
            stateRequired = newState;
            break;
        case shooting:
            stateRequired = newState;
            break;
        case coolDown:
            std::cout << "\033[1;31mForbidden state to set.\033[m\n";
            break;
        case error:
            std::cout << "\033[1;31mForbidden state to set.\033[m\n";
            break;
    }
}

bool Control::remoteControl(bool grab) {
    if(remote != grab){
        remoteRequired = grab;
        return true;
    }
    return false;
}

bool Control::remoteRequest() {
    bool send;
    if(remoteRequired){
        send = chatter->sendCmd(Chatter::cmd::grabControl);
    }else{
        send = chatter->sendCmd(Chatter::cmd::releaseControl);
    }
    if(send){
        Responce resp = chatter->recvResp();
        if(resp.success){
            return true;
        }else{
            printf("No connection to laser!\n");
        }
    }else{
        printf("No connection to MOXA!\n");
    }
    connected = false;
    return false;
}

bool Control::stateRequest() {
    bool send;
    switch (stateRequired) {
        case State::idle:
            send = chatter->sendCmd(Chatter::cmd::idle);
            timeLock = false;
            break;
        case shooting:
            if(stateCurrent == State::readyToShoot){
                send = chatter->sendCmd(Chatter::cmd::shoot);
                break;
            }
        case readyToShoot:
            std::cout << "send warm-up cmd" << std::endl;
            send = chatter->sendCmd(Chatter::cmd::readyToShoot);
            if(stateCurrent == State::shooting){
                break;
            }
            timeLock = true;
            timeout = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) +
                    std::chrono::seconds(readyTimeout);
            std::thread([this]() {
                sleep(readyTimeout);
                timeLock = false;
                stateCurrent = State::readyToShoot;
                printf("Ready to shoot unlocked.\n");
            }).detach();
            break;
        default:
            std::cout << "\033[1;31mBug in stateRequest().\033[m\n";
            break;
    }
    if(send){
        Responce resp = chatter->recvResp();
        if(resp.success){
            return true;
        }else{
            printf("No connection to laser!\n");
        }
    }else{
        printf("No connection to MOXA!\n");
    }
    connected = false;
    return false;
}
