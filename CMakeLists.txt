
cmake_minimum_required(VERSION 3.21)
project(PROTOTYPE C)

set(GLFW_LIB "" CACHE STRING "path to glfw statically linked library")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(prototype)

include_directories(
	${CMAKE_SOURCE_DIR}/application/
	${CMAKE_SOURCE_DIR}/core/libs/
	${CMAKE_SOURCE_DIR}/core/
)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
	${CMAKE_SOURCE_DIR}/core/renderer/*.c
	${CMAKE_SOURCE_DIR}/core/backend/*.c
	${CMAKE_SOURCE_DIR}/core/utils/*.c
	${CMAKE_SOURCE_DIR}/core/files/*.c
	${CMAKE_SOURCE_DIR}/core/libs/*.c
	${CMAKE_SOURCE_DIR}/core/math/*.c
	${CMAKE_SOURCE_DIR}/core/res/*.c
	
	${CMAKE_SOURCE_DIR}/application/*.c
)

if (UNIX)
    find_package(Threads REQUIRED)
    find_package(X11 REQUIRED)
    
    target_link_libraries(prototype PRIVATE ${CMAKE_THREAD_LIBS_INIT} ${X11_LIBRARIES} ${CMAKE_DL_LIBS})
endif()
if (!GLFW_LIB)
	message(FATAL_ERROR "statically linked GLFW library could not be found")
endif()

find_package(OpenGL REQUIRED)

target_sources(prototype PRIVATE 
	${CMAKE_SOURCE_DIR}/core/main.c
	${SRC_FILES}
	${CMAKE_SOURCE_DIR}/core/libs/glad.c
)
target_compile_options(prototype PRIVATE -Wall -Werror -pedantic)
target_link_options(prototype PUBLIC -fPIE -fPIC -lm)

target_link_libraries(prototype PRIVATE ${OPENGL_gl_LIBRARY} ${GLFW_LIB})

