cmake_minimum_required(VERSION 3.13)
project(SocketProgramming)

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS -pthread)

add_executable(server server.cpp)
add_executable(client client.cpp)