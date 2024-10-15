cmake_minimum_required(VERSION 3.22.1)
project(ReactorServer)

# 设置构建类型为 Debug
set(CMAKE_BUILD_TYPE Debug)

file(GLOB MY_SOURCES ${CMAKE_SOURCE_DIR}/*.c)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# set(CMAKE_VERBOSE_MAKEFILE ON)

add_executable(main ${MY_SOURCES})