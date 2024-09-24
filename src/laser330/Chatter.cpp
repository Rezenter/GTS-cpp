//
// Created by nz on 29.04.2020.
//

#include "Chatter.h"

    constexpr char Chatter::cmd::grabControl[10]; //grab remote Control
    constexpr char Chatter::cmd::releaseControl[10]; //release remote Control
    constexpr char Chatter::cmd::idle[10]; //state 2, thermostabilisation, default, safe state
    constexpr char Chatter::cmd::readyToShoot[10]; //state 3, desync generation
    constexpr char Chatter::cmd::shoot[10]; //state 4, generation
    constexpr char Chatter::cmd::status[10]; //get status
    constexpr char Chatter::cmd::error[10]; //get errors

Chatter::Chatter(){
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    online = establishConnection();
    std::cout << "chatter constructed" << std::endl;
}

bool Chatter::establishConnection(){
    /*
    crc(Chatter::cmd::grabControl, 10);
    crc(Chatter::cmd::releaseControl, 10);
    crc(Chatter::cmd::idle, 10);
    crc(Chatter::cmd::readyToShoot, 10);
    crc(Chatter::cmd::shoot, 10);
    crc(Chatter::cmd::status, 10);
    crc(Chatter::cmd::error, 10);

    exit(111);
*/
    printf("Connection attempt %u:\n", ++connectionAttempt);
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("    Socket creation error \n");
        return false;
    }

    if (inet_pton(AF_INET, ipAddr, &serv_addr.sin_addr) <= 0) {  //move upper
        printf("    Invalid address/ Address not supported \n");
        close(sock);
        return false;
    }

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("    Connection Failed \n");
        close(sock);
        return false;
    }
    printf("    MOXA OK\n");
    connectionAttempt = 0;
    return true;
};

bool Chatter::sendCmd(const char* packet) {
    if(online) {
        bytesSend = send(sock , packet, packetLength , 0 );
        if(bytesSend != packetLength){
            online = false;
            return false;
        }
        return true;
    }else{
        if(sock){
            printf("Closing socket\n");
            read( sock , buffer, 16);
            close(sock);
        }
        online = establishConnection();
        return false;
    }
}

Responce Chatter::recvResp() {
    if(online){
        bytesRecv = read( sock , buffer, 16);

/*
        printf("\tcount = %d, packet = ", bytesRecv);
        for(int i = 0; i < bytesRecv; i++){
            if(buffer[i] == '\r'){
                std::cout << "<CR>";
            }else{
                std::cout << buffer[i];
            }
        }
        std::cout << std::endl;
*/
        if(bytesRecv > 0){
            bool changeState;
            unsigned short length;
            switch (buffer[0]) {
                case statusPrefix:
                    length = statusPacketLength;
                    changeState = false;
                    break;
                case statePrefix:
                    length = statePacketLength;
                    changeState = true;
                    break;
                default:
                    printf("Unknown prefix %c.\n", buffer[0]);
                    return {false, 0};
            }
            //std::cout << "expected length = " << length << std::endl;
            if(true || (bytesRecv == length &&
               buffer[length - 1] == endOfPacket)){
                if(!crc(buffer, length - 3)){
                    //printf("Incorrect crc!\n");
                    //return {false, 0};
                }
                if(changeState){
                    long payload = strtol(buffer + 1, NULL, 16);
                    //printf("payload = %d\n", payload);
                    switch (payload) {
                        case 0:
                            std::cout << "Command executed." << std::endl;
                            return {true, payload};
                        case 1:
                            //std::cout << "Command put in queue." << std::endl;
                            return {true, payload};
                        /*case 2:
                            //std::cout << "Command declined." << std::endl;
                            return {false, payload};
                        case 3:
                            //std::cout << "Command has incorrect parameter." << std::endl;
                            return {false, payload};
                        case 4:
                            //td::cout << "Command forbidden." << std::endl;
                            return {false, payload};
                        case 5:
                            //std::cout << "Command not parsed: reason unknown." << std::endl;
                            return {false, payload};
                        case 6:
                            //std::cout << "Command has wrong format." << std::endl;
                            return {false, payload};
                        case 7:
                            //std::cout << "Command has bad crc." << std::endl;
                            return {false, payload};
                        */
                        default:
                            std::cout << "Command was rejected!" << std::endl;
                            return {false, payload};
                    }
                }else {
                    for(int f = 0; f < 4; f++){
                        buffer1[f] = buffer[f + 1];
                    }
                    long payload1 = strtol(buffer1, NULL, 16);
                    long payload2 = strtol(buffer + 1 + 4 + 1, NULL, 16);
                    switch (payload1) {
                        case 0x0700:
                            return {true, payload2};
                        case 0x0800:
                            printf("error report\n");
                            return {true, 0};
                        default:
                            printf("Unknown payload %x, %x.\n", payload1, payload2);
                            return {false, 0};
                    }
                }
            }
        }
        online = false;
        printf("Warning! Received packet error.\n");
        return {false, 0};
    }
    printf("WTF? Can not request offline!\n");
    return {false, 0};
}

bool Chatter::crc(char* packet, size_t length) {
    unsigned short res = 0;
    for(size_t i = 0; i < length; i++){
        res += packet[i];
    }
    res &= 0xff;
    char s[2];
    //printf("%x\n", res);
    sprintf(s, "%x", res);
    return s[0] == packet[length] && s[1] == packet[length + 1];
}

Chatter::~Chatter() {
    if(sock){
        printf("Closing MOXA socket\n");
        close(sock);
    }
}
