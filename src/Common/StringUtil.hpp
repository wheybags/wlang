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
  inline std::string space(int32_t count) { return std::string(count, ' '); }

#ifdef WIN32
  std::string wstringToUtf8(std::wstring_view wstr);
  std::wstring utf8ToWstring(std::string_view utf8);
#endif
}