cmake_minimum_required(VERSION 3.17)
project(wlang)

set(CMAKE_CXX_STANDARD 20)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} -Wall -Wextra -pedantic -Wimplicit-fallthrough -Wunused -Wuninitialized -Wno-unused-function")
endif()

add_subdirectory(GrammarTool)

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS *.cpp *.hpp)
add_executable(wlang ${SOURCE_FILES})