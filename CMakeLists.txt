cmake_minimum_required(VERSION 3.28)
project(MPP_D1 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -ffast-math")

include_directories(src)

add_executable(generator src/d1/Generator.cpp)
add_executable(sequential src/d1/Sequential.cpp)