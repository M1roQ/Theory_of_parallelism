cmake_minimum_required(VERSION 3.10)

project(MatrixVectorBenchmark)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
# Включить поддержку OpenMP
find_package(OpenMP REQUIRED)

set(SOURCE_FILES
    main.cpp
)

# исполняемый файл
add_executable(MatrixVectorBenchmark ${SOURCE_FILES})

target_link_libraries(MatrixVectorBenchmark OpenMP::OpenMP_CXX)