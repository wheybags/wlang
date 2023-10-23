cmake_minimum_required(VERSION 3.17)
project(wlang)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} -Wall -Wextra -pedantic -Wimplicit-fallthrough -Wunused -Wuninitialized -Wno-unused-function")

add_subdirectory(GrammarTool)

add_executable(wlang main.cpp StringUtil.hpp StringUtil.cpp Tokeniser.hpp Tokeniser.cpp Assert.hpp Parser.cpp Parser.hpp Ast.hpp Ast.cpp PlainCGenerator.cpp PlainCGenerator.hpp)