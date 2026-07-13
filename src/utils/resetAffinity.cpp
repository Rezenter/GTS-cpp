//
// Created by user on 01.11.2025.
//

#include <windows.h>
#include <tchar.h>
#include <psapi.h>
#include "iostream"


int main() {
    //mg_log_set(1);
    std::cout << "reset affinity for Windows processes to all cores" << std::endl;

    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;
    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) ){
        return 1;
    }
    cProcesses = cbNeeded / sizeof(DWORD);
    size_t ok = 0;
    for (i = 0; i < cProcesses; i++ ){
        if( aProcesses[i] != 0 ){
            HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                           PROCESS_ALL_ACCESS,
                                           FALSE, aProcesses[i] );
            ok += SetProcessAffinityMask(hProcess, 0b1111111111111111);
            std::cout  << aProcesses[i] << ' ' << SetProcessAffinityMask(hProcess, 0b1111111111111111) << std::endl;
            CloseHandle( hProcess );
        }
    }
    std::cout << "moved " << ok << "/" << cProcesses << std::endl;
    return 0;
}