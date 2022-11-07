cmake_minimum_required(VERSION 3.14)
project(glad)

# C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(glad STATIC src/gl.c)
target_include_directories(glad PUBLIC include/)