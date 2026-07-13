//
// Created by user on 19.01.2026.
//

#include <iostream>
#include <WinSock2.h>

#include "string"
#include <filesystem>
#include <fstream>

#include "json.hpp"

#include <tchar.h>
#include <psapi.h>
#include <chrono>
#include "array"
#include "cmath"
#include <limits>

using Json = nlohmann::json;
int main() {

    std::cout << "CTS c++ time measure" << std::endl;

    DWORD aProcesses[1024], cbNeeded, cProcesses;

    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) ){
        return 1;
    }
    cProcesses = cbNeeded / sizeof(DWORD);
    size_t ok = 0;
    for (unsigned int i = 0; i < cProcesses; i++ ){
        if( aProcesses[i] != 0 ){
            HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                           PROCESS_ALL_ACCESS,
                                           FALSE, aProcesses[i] );
            ok += SetProcessAffinityMask(hProcess, 0b1110000000000000);
            //std::cout  << aProcesses[i] << ' ' << SetProcessAffinityMask(hProcess, 0b1110000000000000) << std::endl;
            CloseHandle( hProcess );
        }
    }


    std::cout << "moved " << ok << "/" << cProcesses << std::endl;
    std::cout << "Process affinity: " << ' ' << SetProcessAffinityMask(GetCurrentProcess(), 0b0000000000000001) << std::endl;
    std::cout << "process realtime: " << ' ' << SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS) << std::endl;

    const unsigned long long mask = 0b0000000000000001;
    std::cout << SetThreadAffinityMask(GetCurrentThread(), mask) << std::endl;
    std::cout << "Main thread " << GetCurrentThreadId() << " , affinity: " << ' ' << SetThreadAffinityMask(GetCurrentThread(), mask) << std::endl;



    std::array<std::array<std::array<unsigned short, 1024>, 16>, 100>* result;
    result = new std::array<std::array<std::array<unsigned short, 1024>, 16>, 100>;

    std::ifstream file;
    file.open({R"(d:\data\db\plasma\raw\46748\0.msgpk)"}, std::ios::in | std::ios::binary);
    {
        Json data = Json::from_msgpack(file);

        for(int eventInd = 0; eventInd < 100; eventInd++){
            for(int ch_ind = 0; ch_ind < 16; ch_ind++){
                for(int cell_ind = 0; cell_ind < 1024; cell_ind++){
                    (*result)[eventInd][ch_ind][cell_ind] = data[eventInd+1]["ch"][ch_ind][cell_ind];
                }
            }
        }

    }
    file.close();

    std::array<float, 6219> expectedTe;
    std::array<std::array<float, 6219>, 5> expectedF;

    file.open({R"(d:\data\db\calibration\expected\2025.10.12__full.json)"}, std::ios::in);
    {
        Json data = Json::parse(file);
        for(uint16_t i = 0; i < 6219; i++){
            expectedTe[i] = data["T_arr"][i];
            for(uint8_t chInd = 0; chInd < 5; chInd++){
                expectedF[chInd][i] = data["poly"]["51"]["expected"][chInd][i];
            }
        }
    }
    file.close();

    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;

    double res = 1;
    for(int i = 1; i < 1000000000; i++){
        res = res*std::sin(i) / 3.14;   //preload CPU
    }
    std::cout << res << std::endl << std::endl;


    uint32_t repeat_max = 100000;
    int_fast16_t max = 0;
    uint_fast16_t maxInd = 0;
    int_fast16_t curr = 0;
    int_fast16_t top = 0;
    int_fast16_t bot = 0;

    std::array<int_fast16_t, 16> zeros {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    std::array<int_fast16_t, 16> stds  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    std::array<int_fast16_t, 16> ints  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


    auto t1 = high_resolution_clock::now();
    for(uint_fast32_t repeat = 0; repeat < repeat_max; repeat++){
        //std::cout << repeat << std::endl;
        for(uint_fast16_t eventInd = 0; eventInd < 100; eventInd++) {


            for (uint_fast16_t cell_ind = 0; cell_ind < 1023; cell_ind++) {
                curr = (*result)[eventInd][0][cell_ind + 1] - (*result)[eventInd][0][cell_ind];
                if (curr > max) {
                    maxInd = cell_ind;
                    max = curr;
                } else if (max > curr + 50) {
                    break;
                };
            }

            for(uint_fast8_t ch_ind = 1; ch_ind < 16; ch_ind++){
                for (uint_fast16_t cell_ind = 0; cell_ind < 300; cell_ind++) {
                    zeros[ch_ind] += (*result)[eventInd][ch_ind][cell_ind];
                }
                zeros[ch_ind] /= 300;
            }

            for(uint_fast8_t ch_ind = 1; ch_ind < 16; ch_ind++){
                for (uint_fast16_t cell_ind = 0; cell_ind < 300; cell_ind++) {
                    stds[ch_ind] += ((*result)[eventInd][ch_ind][cell_ind] - zeros[ch_ind])*((*result)[eventInd][ch_ind][cell_ind] - zeros[ch_ind]); //^2
                }
                stds[ch_ind] = 937.5 / stds[ch_ind]; //937.5 = 300/0.32; 0.32 = 0.61*0.52 see later
                //already squared!

                for (uint_fast16_t cell_ind = 0; cell_ind < 50; cell_ind++) {
                    //add boundary check!
                    ints[ch_ind] += (*result)[eventInd][ch_ind][cell_ind + maxInd + 30];
                }
                ints[ch_ind] = ((ints[ch_ind] - 50 * zeros[ch_ind]) * 0.61 - 7500) * 0.52; //0.61 = 2500/4096; 7500 = 150*50
                //signal = -150 + v * 2500 / 4096  //v = cell
                //photoelectrons = 2 * integral * 1e-3 * 1e-9 / (self.config['preamp']['apdGain'] *
                //                                                       phys_const.q_e *
                //                                                       self.config['preamp']['feedbackResistance'] *
                //                                                       sp_ch['fast_gain'] *
                //                                                       matching_gain) = *0.52
                //err2 = math.pow(pre_std * matching_gain * 4 / sp_ch['fast_gain'], 2) * 6715 * 0.0625 - 1.14e4 * 0.0625
            }


            for(uint_fast8_t poly = 0; poly < 3; poly++){
                max = UINT_LEAST16_MAX;
                for(uint_fast16_t t_ind = 0; t_ind < 100; t_ind++){
                    top = (ints[1 + poly*5] * expectedF[0][t_ind]) * stds[1 + poly*5] +
                            (ints[2 + poly*5] * expectedF[1][t_ind]) * stds[2 + poly*5] +
                            (ints[3 + poly*5] * expectedF[2][t_ind]) * stds[3 + poly*5] +
                            (ints[4 + poly*5] * expectedF[3][t_ind]) * stds[4 + poly*5] +
                            (ints[5 + poly*5] * expectedF[4][t_ind]) * stds[5 + poly*5];
                    bot = expectedF[0][t_ind]*expectedF[0][t_ind] * stds[1 + poly*5] +
                            expectedF[1][t_ind]*expectedF[1][t_ind] * stds[2 + poly*5] +
                            expectedF[2][t_ind]*expectedF[2][t_ind] * stds[3 + poly*5] +
                            expectedF[3][t_ind]*expectedF[3][t_ind] * stds[4 + poly*5] +
                            expectedF[4][t_ind]*expectedF[4][t_ind] * stds[5 + poly*5];
                    curr = (ints[1 + poly*5] - top*expectedF[0][t_ind]/bot)*(ints[1 + poly*5] - top*expectedF[0][t_ind]/bot)*stds[1 + poly*5] +
                            (ints[2 + poly*5] - top*expectedF[1][t_ind]/bot)*(ints[2 + poly*5] - top*expectedF[1][t_ind]/bot)*stds[2 + poly*5] +
                            (ints[3 + poly*5] - top*expectedF[2][t_ind]/bot)*(ints[3 + poly*5] - top*expectedF[2][t_ind]/bot)*stds[3 + poly*5] +
                            (ints[4 + poly*5] - top*expectedF[3][t_ind]/bot)*(ints[4 + poly*5] - top*expectedF[3][t_ind]/bot)*stds[4 + poly*5] +
                            (ints[5 + poly*5] - top*expectedF[4][t_ind]/bot)*(ints[5 + poly*5] - top*expectedF[4][t_ind]/bot)*stds[5 + poly*5];
                    if(curr < max){
                        max = curr;
                        maxInd = t_ind;
                    }
                }
            }


            /*
             def calc_chi2(N_i, sigm2_i, f_i):
                res = 0
                top_sum = 0
                bot_sum = 0
                for ch in range(len(N_i)):
                    top_sum += (N_i[ch] * f_i[ch]) / sigm2_i[ch]
                    bot_sum += math.pow(f_i[ch], 2) / sigm2_i[ch]
                for ch in range(len(N_i)):
                    res += math.pow(N_i[ch] - (top_sum * f_i[ch] / bot_sum), 2) / sigm2_i[ch]
                return res


             chi2 = float('inf')
            N_i = []
            sigm2_i = []

            for ch in channels:
                N_i.append(event['ch'][ch]['ph_el'])
                sigm2_i.append(math.pow(event['ch'][ch]['err'], 2))
            min_index = -1
            for i in range(len(self.expected['T_arr'])):
                f_i = [self.expected['poly'][poly]['expected'][ch][i] for ch in channels]
                current_chi = calc_chi2(N_i, sigm2_i, f_i)
                if current_chi < chi2:
                    min_index = i
                    chi2 = current_chi

             f2_sum = 0
            df_sum = 0
            fdf_sum = 0
            nf_sum = 0
            for ch in range(len(channels)):
                #if poly == 5 and event_ind == 60:
                #    print('N_i %.1e, f %.1e, sigma2 %.1e' % (N_i[ch], f[ch], sigm2_i[ch]))
                f2_sum += math.pow(f[ch], 2) / sigm2_i[ch]
                df_sum += math.pow(df[ch], 2) / sigm2_i[ch]
                fdf_sum += f[ch] * df[ch] / sigm2_i[ch]
                nf_sum += N_i[ch] * f[ch] / sigm2_i[ch]
            fdf_sum = math.pow(fdf_sum, 2)

             A = self.absolute['A'][poly] * math.pow(phys_const.r_o, 2) * self.result['config']['laser'][0]['wavelength'] * 1e-9 / (phys_const.q_e * self.result['config']['preamp']['apdGain'])


            mult = nf_sum / f2_sum
            Terr2 = math.pow(A * E * n_e, -2) * f2_sum / (f2_sum * df_sum - fdf_sum)
            nerr2 = math.pow(A * E, -2) * df_sum / (f2_sum * df_sum - fdf_sum)


            res = filter({
                'index': min_index,
                'min': self.expected['T_arr'][min_index],
                'ch': channels,
                'chi2': (left['chi'] + right['chi']) * 0.5,
                'T': (left['t'] + right['t']) * 0.5,
                'Terr': math.sqrt(Terr2),
                'n': n_e,
                'n_err': math.sqrt(nerr2),
                'mult': mult,
                'error': None
            })
             */
        }
    }

    auto t2 = high_resolution_clock::now();
    std::cout << maxInd << ' ' << max << ' ' << curr << std::endl;

    for(uint_fast8_t ch_ind = 1; ch_ind < 16; ch_ind++){
        std::cout << zeros[ch_ind] << ' ' << stds[ch_ind] << ' ' << ints[ch_ind] << std::endl;
    }

    duration<double, std::milli> ms_double = t2 - t1;
    std::cout << '\n' << ms_double.count() << ' ' << ms_double.count()/(repeat_max*100) << "ms\n";

    std::cout << "Code OK" << std::endl;

    delete[] result;

    return 0;
}