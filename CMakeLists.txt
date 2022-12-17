cmake_minimum_required(VERSION 3.24)
project(bfs)

set(CMAKE_CXX_STANDARD 17)

add_executable(bfs main.cpp)

find_package(Threads REQUIRED)
find_package(Parlay REQUIRED)
target_link_libraries(bfs PUBLIC Parlay::parlay)
