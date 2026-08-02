
# install raylib statically, from source code. Simple.

cmake_minimum_required(VERSION 3.21)
project(pessoa C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(pessoa)

target_sources(pessoa PRIVATE ${CMAKE_SOURCE_DIR}/main.c)

target_include_directories(pessoa PRIVATE ${CMAKE_SOURCE_DIR}/libs/raylib/src/)

target_compile_options(pessoa PRIVATE -Wall -Werror -pedantic -Wno-format -Wno-unused-function)
target_link_libraries(pessoa PRIVATE ${CMAKE_SOURCE_DIR}/libs/raylib/src/libraylib.a m GL pthread dl rt X11)
