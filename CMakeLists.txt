cmake_minimum_required(VERSION 3.6)
project(nes-emu)

# C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(libs/glad)
add_subdirectory(libs/imgui)

set(NES_SRC_FILES 
  src/nes/apu.cpp
  src/nes/cartridge.cpp
  src/nes/cpu.cpp
  src/nes/joypad.cpp
  src/nes/nes.cpp
  src/nes/ppu.cpp
  src/nes/waveform_capture.cpp
)

add_compile_definitions(_USE_MATH_DEFINES)
add_executable(nes-emu src/main.cpp src/renderer.cpp src/audio.cpp ${NES_SRC_FILES})
add_executable(nestest src/nestest.cpp ${NES_SRC_FILES})
target_link_libraries(nes-emu PRIVATE imgui)
target_include_directories(nes-emu PRIVATE src/)
target_include_directories(nestest PRIVATE src/)

if (EMSCRIPTEN)
  set_target_properties(nes-emu
        PROPERTIES SUFFIX ".html"
        COMPILE_FLAGS "-s USE_SDL=2 -fexceptions"
        LINK_FLAGS "-s USE_GLFW=3 -s USE_WEBGL2=1 -s USE_SDL=2 -fexceptions \
                    --preload-file ${CMAKE_SOURCE_DIR}/assets/nestest.nes@/assets/nestest.nes \
                    --shell-file ${CMAKE_SOURCE_DIR}/template.html")
else()
  include(FetchContent)
  set(FETCHCONTENT_QUIET FALSE)

  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG 3.3.8
    GIT_PROGRESS TRUE
  )
  FetchContent_MakeAvailable(glfw)

  FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG release-2.24.0
    GIT_PROGRESS TRUE
  )
  FetchContent_MakeAvailable(SDL2)

  target_link_libraries(nes-emu PRIVATE 
    glfw opengl32 glad
    SDL2main SDL2-static)
  target_include_directories(nes-emu PRIVATE 
    ${glfw_SOURCE_DIR}/include
    ${sdl2_SOURCE_DIRS}/include)
endif()