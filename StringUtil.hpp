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
  bool endsWith(std::string_view str, std::string_view suffix);
  static std::string space(int32_t count) { return std::string(count, ' '); }
}