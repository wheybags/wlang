file(GLOB SOURCE_FILES CONFIGURE_DEPENDS *.cpp *.hpp)
file(GLOB COMMON_SOURCE_FILES CONFIGURE_DEPENDS ../Common/*.cpp ../Common/*.hpp)
add_executable(GrammarTool ${SOURCE_FILES} ${COMMON_SOURCE_FILES})