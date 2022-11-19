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

set(SRC_FILES 
  cartridge.cpp
  cpu.cpp
  joypad.cpp
  ppu.cpp
  apu.cpp
  waveform_capture.cpp
)

add_compile_definitions(_USE_MATH_DEFINES)
add_executable(nes-emu main.cpp renderer.cpp audio.cpp ${SRC_FILES})
add_executable(nestest nestest.cpp ${SRC_FILES})
target_link_libraries(nes-emu PRIVATE imgui)

if (EMSCRIPTEN)
  set_target_properties(nes-emu
        PROPERTIES SUFFIX ".html"
        COMPILE_FLAGS "-fexceptions"
        LINK_FLAGS "-s USE_GLFW=3 -s USE_WEBGL2=1 -fexceptions \
                    --preload-file ${CMAKE_SOURCE_DIR}/roms/nestest.nes@/roms/nestest.nes \
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
    openal-soft
    GIT_REPOSITORY https://github.com/kcat/openal-soft
    GIT_TAG 1.22.2
    GIT_PROGRESS TRUE
  )
  FetchContent_MakeAvailable(openal-soft)

  target_link_libraries(nes-emu PRIVATE glfw opengl32 glad OpenAL)
  target_include_directories(nes-emu PRIVATE ${glfw_SOURCE_DIR}/include ${openal-soft_SOURCE_DIR}/include)
endif()