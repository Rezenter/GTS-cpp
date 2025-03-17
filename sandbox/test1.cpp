//
// Created by user on 30.10.2024.
//

#include <iostream>

int main(){
    bool stop = false;
    bool stopped = false;
    int i = 0;
    std::cout << stop << ' ' << stopped << ' ' << i << std::endl;
    while(!(stop or stopped)){
        std::cout << stop << ' ' << stopped << ' ' << i << std::endl;
        if(i < 10){
            i++;
        }else{
            stopped = true;
        }
    }
    std::cout << stop << ' ' << stopped << ' ' << i << std::endl;
    return 0;
};