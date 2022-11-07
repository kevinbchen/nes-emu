cmake_minimum_required(VERSION 3.6)
project(nes-emu)

# C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(libs/glad)

set(SRC_FILES 
  cartridge.cpp
  cpu.cpp
  ppu.cpp
)

add_executable(nes-emu main.cpp renderer.cpp ${SRC_FILES})
add_executable(nestest nestest.cpp ${SRC_FILES})

if (EMSCRIPTEN)
  set_target_properties(nes-emu
        PROPERTIES SUFFIX ".html"
        COMPILE_FLAGS "-fexceptions"
        LINK_FLAGS "-s USE_GLFW=3 -s USE_WEBGL2=1 -fexceptions --preload-file ${CMAKE_SOURCE_DIR}/roms@/roms --shell-file ${CMAKE_SOURCE_DIR}/template.html ")
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

  target_link_libraries(nes-emu PRIVATE glfw opengl32 glad)
  target_include_directories(nes-emu PRIVATE ${glfw_SOURCE_DIR}/include)
endif()