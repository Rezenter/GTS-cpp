//
// Created by user on 27.09.2024.
//

#ifndef GTS_CORE_STOPPABLE_H
#define GTS_CORE_STOPPABLE_H

#include <thread>
#include <chrono>
#include <mutex>

class Stoppable{
private:
    mutable std::mutex mutex;

protected:
    virtual bool payload() = 0; //this one is pure virtual. return isFinished
    virtual void beforePayload(){};
    virtual void afterPayload(){};
    bool stop = false;

public:
    std::thread associatedThread;
    virtual ~Stoppable(){
        this->requestStop();
        this->associatedThread.join();
    };

    void requestStop(){
        this->mutex.lock();
        this->stop = true;
        this->mutex.unlock();
    };

    void run(){
        this->mutex.lock();
        this->stop = false;
        this->mutex.unlock();

        this->beforePayload();
        while(true){
            this->mutex.lock();
            if(this->stop){
                this->mutex.unlock();
                break;
            }else {
                this->mutex.unlock();
                if (this->payload()) {
                    break;
                }
            }
        }
        this->afterPayload();
    };
};

#endif //GTS_CORE_STOPPABLE_H
