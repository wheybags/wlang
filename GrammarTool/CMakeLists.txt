file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS *.cpp *.hpp)
add_executable(GrammarTool ${SOURCE_FILES} ../StringUtil.cpp ../StringUtil.hpp ../Assert.hpp)