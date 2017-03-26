cmake_minimum_required(VERSION 3.4)
project(producer_comsumer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    pro-con.cpp)

add_executable(producer_comsumer ${SOURCE_FILES})