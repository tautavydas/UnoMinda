cmake_minimum_required(VERSION 3.9)
project(CAN_driver)
set(CMAKE_CXX_STANDARD 20)

add_executable(main source/main.cpp)
target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)