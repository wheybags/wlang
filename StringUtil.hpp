#pragma once
#include <string>

namespace Str
{
  bool isSpace(char c);
  bool isAlpha(char c);
  bool isNumeric(char c);
  bool isNumeric(std::string_view str);
  bool isAlphaNumeric(char c);
  bool startsWith(std::string_view str, std::string_view prefix);
}