//
// Created by ts_group on 14.11.2022.
//

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <cstdio>
#include <stdio.h>
#include <sys/socket.h>
#include<arpa/inet.h>

union Packet{
    char bytes[2];
    unsigned short integer;
};

int main([[maybe_unused]] int argc,[[maybe_unused]] char* argv[]) {
    printf("Initial\n");
    int file;
    int i2cAddr1 = 0x61;
    int i2cAddr2 = 0x60;
    int adapter_nr = 1;
    char filename[20];
    snprintf(filename, 19, "/dev/i2c-%d", adapter_nr);
    file = open(filename, O_RDWR);
    if (file < 0) {
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, i2cAddr1) < 0) {
        return 2;
    }
    if (ioctl(file, I2C_SLAVE, i2cAddr2) < 0) {
        return 2;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1){
        printf("Could not create socket.");
    }
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr("192.168.10.56");
    addr.sin_family = AF_INET;
    addr.sin_port = htons( 8080 );

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connection error");
        return 3;
    }
    char buffer[16];
    Packet packet;
    printf("listening...\n");
    while(1) {
        if(recv(sockfd, buffer, 16, MSG_DONTWAIT) > 3) {
            ioctl(file, I2C_SLAVE, i2cAddr1);
            packet.bytes[0] = buffer[1] & 0x0F;
            packet.bytes[1] = buffer[0];
            if (write(file, packet.bytes, 2) != 2) {
                return 5;
            }
            ioctl(file, I2C_SLAVE, i2cAddr2);
            packet.bytes[0] = buffer[3] & 0x0F;
            packet.bytes[1] = buffer[2];
            if (write(file, packet.bytes, 2) != 2) {
                return 5;
            }
            //printf("recv : %d\n", packet.integer);
        }
    }
    close(sockfd);
    return 0;
}