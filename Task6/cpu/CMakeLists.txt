cmake_minimum_required(VERSION 3.10)
project(cpu_project CXX)

set(CMAKE_CXX_COMPILER pgc++)

set(INFO_FLAGS "-Minfo=all")
set(LIBS "-lboost_program_options")

set(SOURCE main_cpu.cpp)

add_executable(serial ${SOURCE})
target_compile_options(serial PRIVATE -acc=host ${INFO_FLAGS})
target_link_libraries(serial PRIVATE ${LIBS})

add_executable(multi ${SOURCE})
target_compile_options(multi PRIVATE -acc=multicore ${INFO_FLAGS} -mp)
target_link_libraries(multi PRIVATE ${LIBS})
target_link_options(multi PRIVATE -acc=multicore -mp)
