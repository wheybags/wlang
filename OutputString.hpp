#pragma once
#include <string>

class OutputString
{
public:
  void appendLine(std::string_view line = "");

  void indent() { indentCount += 2; }
  void outdent() { indentCount -= 2; }

public:
  int32_t indentCount = 0;
  std::string str;
};