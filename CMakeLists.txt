cmake_minimum_required(VERSION 3.21)
project(GTS-core)

set(CMAKE_CXX_STANDARD 23)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2")


#SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${BUILD_ARCH} -Wall" )
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${BUILD_ARCH}" )
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${BUILD_ARCH}")
#SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${BUILD_ARCH} -static -static-libgcc -static-libstdc++")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include_directories(${CMAKE_SOURCE_DIR} include lib src)


add_executable(GTS-core src/server.cpp
        include/mongoose.c include/mongoose.h include/json.hpp
        src/laser330/Laser330.cpp src/laser330/Laser330.h
        src/Diagnostics/Diagnostics.cpp src/Diagnostics/Diagnostics.h
        src/laser330/Coolant.cpp src/laser330/Coolant.h
        src/Caen/AllBoards.cpp src/Caen/AllBoards.h
        src/Caen/Link.cpp src/Caen/Link.h
        src/Diagnostics/Storage.cpp src/Diagnostics/Storage.h
        )
target_link_libraries(GTS-core ${CMAKE_CURRENT_SOURCE_DIR}/lib/CAENDigitizer.lib wsock32 ws2_32 )

add_executable(test1 sandbox/test1.cpp)

add_executable(testOphir src/Ophir/debug.cpp src/Ophir/OphirLMMeasurement.h src/Ophir/OphirLMMeasurement.cpp)