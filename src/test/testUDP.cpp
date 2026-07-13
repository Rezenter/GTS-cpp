//
// Created by user on 22.09.2025.
//

#include <WinSock2.h>
#include "iostream"
#include <thread> // Required for std::this_thread::sleep_for
#include <chrono> // Required for std::chrono::seconds, milliseconds, etc.

struct Poly{
    uint32_t R;
    float Te;
    float ne;
    float Te_err;
    float ne_err;
};

struct LaserShot{
    unsigned short shotCount;
    unsigned char polyCount;
    Poly* poly;
};

int main() {
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    int err = WSAStartup(wVersionRequested, &wsaData);

    SOCKET sockfd= socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(6200);
    std::string addr = "100.100.100.4";
    servaddr.sin_addr.s_addr = inet_addr(addr.c_str());



    char* shots;
    unsigned char polyCount = 11;
    std::size_t eventSize = sizeof(short) + sizeof(char) + polyCount*sizeof(Poly);
    shots = new char[eventSize];

    char* ptr = shots;
    unsigned short i = 0;
    std::memcpy(ptr, &i, sizeof(short));
    std::memcpy(ptr + sizeof(short), &polyCount, sizeof(char));
    ptr += sizeof(short) + sizeof(char);
    Poly tmp = Poly{600,
                    60,
                    1.5,
                    5,
                    0.1};
    for(unsigned char polyIndex = 0; polyIndex < polyCount; polyIndex++){
        std::memcpy(ptr, &tmp, sizeof(Poly));
        ptr += sizeof(Poly);
    }

    std::cout << sizeof(short) + sizeof(char) << ' ' << sizeof(short) + sizeof(char) + polyCount*sizeof(Poly) << std::endl;

    while(1){
        std::cout << "send " << sendto(sockfd, shots,
                                       sizeof(short) + sizeof(char) + polyCount*sizeof(Poly),
                                       0, (const struct sockaddr *) &servaddr,
                                       sizeof(servaddr)) << ' ' << WSAGetLastError() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        //std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    closesocket(sockfd);

}