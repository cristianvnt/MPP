cmake_minimum_required(VERSION 3.28)
project(MPP_D1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src)

add_executable(generator src/d1/Generator.cpp)
add_executable(sequential src/d1/Sequential.cpp)