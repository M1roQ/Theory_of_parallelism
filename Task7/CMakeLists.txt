cmake_minimum_required(VERSION 3.10)

# Название проекта
project(task7 LANGUAGES CXX)

set(CMAKE_CXX_COMPILER pgc++)

add_compile_options(-acc=gpu -Minfo=all)
add_link_options(-acc=gpu -Minfo=all)

set(LIBS -cudalib=cublas -lboost_program_options)

add_executable(blas main_blas.cpp)

# Линкуем библиотеки
target_link_libraries(blas PRIVATE ${LIBS})
