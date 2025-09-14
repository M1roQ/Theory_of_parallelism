cmake_minimum_required(VERSION 3.11)

project(SinArray)

set(CMAKE_CXX_STANDARD 14)

#cd Tasks/Task1_sin/build
#cmake -DUSE_FLOAT=OFF .. - double
#cmake -DUSE_FLOAT=On .. / cmake .. - float
#cd build 
#./SinArray
option(USE_FLOAT OFF)

if(USE_FLOAT)
    add_definitions(-DUSE_FLOAT)
endif()

add_executable(SinArray SinArray.cpp)
