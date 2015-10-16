cmake_minimum_required(VERSION 3.3)
project(LibMallocTrace)

set(BUILD_SHARED_LIBS ON)
include_directories(INCLUDE_DIRECTORIES "Heap-Layers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_SHARED_LINKER_FLAGS "-ldl -shared")

add_library(malloctrace libMallocTrace.cpp libMallocTrace.h)
add_executable(tester main.cpp)