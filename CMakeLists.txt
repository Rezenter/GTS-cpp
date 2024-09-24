cmake_minimum_required(VERSION 3.21)
project(GTS-core)

set(CMAKE_CXX_STANDARD 23)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${BUILD_ARCH} -Wall" )
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${BUILD_ARCH}")
#SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${BUILD_ARCH} -static -static-libgcc -static-libstdc++")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include_directories(${CMAKE_SOURCE_DIR} include lib src)


add_executable(GTS-core src/server.cpp
        include/mongoose.c include/mongoose.h src/RequestHandler.cpp src/RequestHandler.h include/json.hpp
        src/DB.cpp src/DB.h
)
target_link_libraries(GTS-core ${CMAKE_CURRENT_SOURCE_DIR}/lib/CAENDigitizer.lib wsock32 ws2_32 )