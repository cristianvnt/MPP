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

add_executable(pthreads src/d2/PThreads.cpp)
target_link_libraries(pthreads pthread)

add_executable(openmp src/d2/OpenMP.cpp)
target_compile_options(openmp PRIVATE -fopenmp)
target_link_libraries(openmp gomp)