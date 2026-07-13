//
// Created by user on 24.02.2026.
//

#include <iostream>

#include "string"
#include <fstream>

#include "array"
#include "cmath"

int main() {

    const unsigned int t11 = 5;
    const unsigned int t12 = 605;
    
    const unsigned int t21 = 649;
    const unsigned int t22 = 800;
    
    
    std::cout << "TS c++ example for FPGA" << std::endl;

    unsigned int eventInd = 15;

    std::array<std::array<std::array<unsigned short, 1024>, 16>, 100>* result;
    result = new std::array<std::array<std::array<unsigned short, 1024>, 16>, 100>;

    std::ifstream file;
    file.open({R"(d:\code\GTS-cpp\src\test\46771.csv)"});
    {
        std::string str;
        for(unsigned int cell = 0; cell < 1024; cell++){
            std::getline(file, str);
            size_t pos = 0;
            for(unsigned  int ch = 0; ch < 15; ch++){
                size_t pos = str.find(", ");
                (*result)[eventInd][ch][cell] = std::stoul(str.substr(0, pos));
                str.erase(0, pos + 2);
            }
            (*result)[eventInd][15][cell] = std::stoul(str);
        }
    }
    file.close();

    std::cout << "read ok" << std::endl;

    int max = 0;
    unsigned int maxInd = 0;
    int curr = 0;

    std::array<int, 16> zeros {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    std::array<float, 16> stds  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    std::array<int, 16> ints  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


    for (unsigned int cell_ind = 0; cell_ind < 1023; cell_ind++) {
        curr = (*result)[eventInd][0][cell_ind + 1] - (*result)[eventInd][0][cell_ind];
        if (curr > max) {
            maxInd = cell_ind;
            max = curr;
        } else if (max > curr + 50) {
            break;
        };

    }

    std::cout << "sync ok" << std::endl;

    for(unsigned int ch_ind = 1; ch_ind < 16; ch_ind++){
        for (unsigned int cell_ind = t11; cell_ind < t12; cell_ind++) {
            zeros[ch_ind] += (*result)[eventInd][ch_ind][cell_ind];
        }
        zeros[ch_ind] /= t12-t11;
    }

    for(unsigned int ch_ind = 1; ch_ind < 16; ch_ind++){
        for (unsigned int cell_ind = t11; cell_ind < t12; cell_ind++) {
            stds[ch_ind] += ((*result)[eventInd][ch_ind][cell_ind] - zeros[ch_ind])*((*result)[eventInd][ch_ind][cell_ind] - zeros[ch_ind]); //^2
        }
        stds[ch_ind] /= t12 - t11;


        for (unsigned int cell_ind = t21; cell_ind < t22; cell_ind++) {
            //ints[ch_ind] += (*result)[eventInd][ch_ind][cell_ind + maxInd + 30];
            ints[ch_ind] += (*result)[eventInd][ch_ind][cell_ind];
        }
    }

    std::cout << "ch:0" << ", maxInd: " << maxInd << std::endl;
    for(unsigned int ch_ind = 1; ch_ind < 16; ch_ind++){
        std::cout << "ch:" << ch_ind << ", zero: " << zeros[ch_ind] << ", SD: " << std::sqrt(stds[ch_ind]) << ", int: " << ints[ch_ind] << std::endl;
    }
    

    std::cout << "Code OK" << std::endl;

    delete[] result;

    return 0;
}