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
    )
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
endif()

add_subdirectory(GrammarTool)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/ParserRules.inl" "${CMAKE_CURRENT_SOURCE_DIR}/ParserRulesDeclarations.inl"
  COMMAND GrammarTool
  DEPENDS GrammarTool
)

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS *.cpp *.hpp)
add_executable(wlang ${SOURCE_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/ParserRules.inl" "${CMAKE_CURRENT_SOURCE_DIR}/ParserRulesDeclarations.inl")
