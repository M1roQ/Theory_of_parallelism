cmake_minimum_required(VERSION 3.10)

project(MyIntegration)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
# Включить поддержку OpenMP
find_package(OpenMP REQUIRED)

set(SOURCE_FILES
    main.cpp
)

# исполняемый файл
add_executable(iteration_method ${SOURCE_FILES})

target_link_libraries(iteration_method OpenMP::OpenMP_CXX)