cmake_minimum_required(VERSION 3.6)
project(nes-emu)

# C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(nes-emu main.cpp cpu.cpp cartridge.cpp)
add_executable(nestest nestest.cpp cpu.cpp cartridge.cpp)