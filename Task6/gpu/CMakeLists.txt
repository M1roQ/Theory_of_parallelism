cmake_minimum_required(VERSION 3.10)
project(GPU_OpenACC LANGUAGES CXX)

# Указать компилятор pgc++
set(CMAKE_CXX_COMPILER pgc++)

# Добавляем флаги компиляции
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -acc=gpu -Minfo=all")

# Указываем исходный файл
add_executable(gpu main_gpu_s2.cpp)

# Подключаем Boost (Program Options)
find_package(Boost REQUIRED COMPONENTS program_options)
target_include_directories(gpu PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(gpu PRIVATE ${Boost_LIBRARIES})
