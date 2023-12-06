add_subdirectory(GrammarTool)

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/ParserRules.inl" "${CMAKE_CURRENT_SOURCE_DIR}/ParserRulesDeclarations.inl"
  COMMAND GrammarTool
  DEPENDS GrammarTool
)

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS *.cpp *.hpp)
add_executable(wlang ${SOURCE_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/ParserRules.inl" "${CMAKE_CURRENT_SOURCE_DIR}/ParserRulesDeclarations.inl")