cmake_minimum_required(VERSION 3.16.3)

project(faris-engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(faris-engine fen.cpp main.cpp movegen.cpp utilities.cpp)
