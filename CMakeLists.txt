cmake_minimum_required(VERSION 3.22.1)
project(ReactorServer)

# 设置构建类型为 Debug
# set(CMAKE_BUILD_TYPE Debug)

file(GLOB MY_SOURCES ${CMAKE_SOURCE_DIR}/src/*.c)
include_directories(${CMAKE_SOURCE_DIR}/include)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(main ${MY_SOURCES})