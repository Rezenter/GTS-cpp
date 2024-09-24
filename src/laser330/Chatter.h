//
// Created by nz on 29.04.2020.
//

#ifndef INC_1064DRIVER_CHATTER_H
#define INC_1064DRIVER_CHATTER_H

//#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstdlib>

struct Responce{
    bool success;
    long payload;
};

class Chatter {
private:
    union Payload{
        uint16_t single;
        uint8_t twin[2];
    };

    bool crc(char* packet, size_t length);
    bool establishConnection();

    bool online = false;
    static const char endOfPacket = '\n';
    //unsigned short port = 50000;
    unsigned short port = 4001;
    unsigned short connectionAttempt = 0;
    unsigned short packetLength = 9;
    unsigned short statePacketLength = 6;
    //unsigned short statusPacketLength = 13;
    unsigned short statusPacketLength = 11;
    int sock;
    int bytesSend;
    int bytesRecv;
    static const char statusPrefix= 'K';
    static const char statePrefix = 'A';

    char* ipAddr = "192.168.127.253";
    char buffer[16] = {0};
    char buffer1[4] = {0};
    struct sockaddr_in serv_addr;

public:
    //wtf?
    constexpr static char status[10] = "J0700 31\n"; //get status
    struct cmd{
        //wrong crc
        constexpr static char grabControl[10] = "S0400 24\n"; //grab remote Control
        constexpr static char releaseControl[10] = "S0200 22\n"; //release remote Control
        constexpr static char idle[10] = "S0012 23\n"; //state 2, thermostabilisation, default, safe state
        constexpr static char readyToShoot[10] = "S100A 32\n"; //state 3, desync generation
        constexpr static char shoot[10] = "S200A 33\n"; //state 4, generation
        constexpr static char status[10] = "J0700 31\n"; //get status
        constexpr static char error[10] = "J0800 1F\n"; //get errors
    };

    Chatter();
    ~Chatter();
    bool sendCmd(const char*);
    Responce recvResp();
};


#endif //INC_1064DRIVER_CHATTER_H
