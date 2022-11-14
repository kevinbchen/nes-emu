cmake_minimum_required(VERSION 3.14)
project(imgui)

# C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB IMGUI_SOURCES *.cpp) 
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC .)
if (NOT EMSCRIPTEN)
  target_link_libraries(imgui PRIVATE glfw)
endif()