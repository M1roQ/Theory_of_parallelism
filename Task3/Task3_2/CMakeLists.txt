cmake_minimum_required(VERSION 3.10)

project(TaskServerClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -pthread")

add_executable(TaskServerClient main.cpp)
target_link_libraries(TaskServerClient pthread)

add_executable(CheckResults check.cpp)
target_link_libraries(CheckResults pthread)
