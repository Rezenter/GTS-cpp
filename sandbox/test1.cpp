//
// Created by user on 30.10.2024.
//

#include <iostream>

#include <cassert>
#include <deque>

int main(){


    std::deque<char> letters{'a', 'b', 'c', 'd'};
    auto b = letters.back();
    std::cout << b << std::endl;
    letters.emplace_back('f');
    std::cout << b << std::endl;
    std::cout << letters.back() << std::endl;

    return 0;
};