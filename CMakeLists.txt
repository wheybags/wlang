cmake_minimum_required(VERSION 3.17)
project(wlang)

set(CMAKE_CXX_STANDARD 20)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(
        -Wall
        -Wextra
        -pedantic
        -Wimplicit-fallthrough
        -Wunused
        -Wuninitialized
        -Wno-unused-function
        -Wswitch
    )
elseif (MSVC)
    add_compile_options(
        /W4
        /w14062 # warn on unhandled enum entries in switch
        /wd4458 # don't warn on shadowing member names
    )
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
endif()

add_subdirectory(src)
