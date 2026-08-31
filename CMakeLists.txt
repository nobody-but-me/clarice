
# install raylib statically, from source code. Simple.

cmake_minimum_required(VERSION 3.21)
project(clarice C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(clarice)

target_sources(clarice PRIVATE ${CMAKE_SOURCE_DIR}/main.c)

target_include_directories(clarice PRIVATE ${CMAKE_SOURCE_DIR}/libs/raylib/src/)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

target_compile_options(clarice PRIVATE -Wall -Werror -pedantic -Wno-format -Wno-unused-function)
target_link_libraries(clarice PRIVATE ${CMAKE_SOURCE_DIR}/libs/raylib/src/libraylib.a m GL pthread dl rt X11)
