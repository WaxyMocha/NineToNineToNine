cmake_minimum_required(VERSION 3.13)
project(NineToNineToNine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3 -march=native -pthread")

add_executable(NineToNineToNine main.cpp)
target_link_libraries(NineToNineToNine stdc++fs)