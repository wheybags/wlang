#include "StringUtil.hpp"
#include <cstring>

namespace Str
{
  bool isSpace(char c)
  {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
  }

  bool isAlpha(char c)
  {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
  }

  bool isNumeric(char c)
  {
    return (c >= '0' && c <= '9');
  }

  bool isNumeric(std::string_view str)
  {
    if (str.empty())
      return false;

    for (char c : str)
    {
      if (!isNumeric(c))
        return false;
    }

    return true;
  }

  bool isAlphaNumeric(char c)
  {
    return isAlpha(c) || (c >= '0' && c <= '9');
  }

  bool startsWith(std::string_view str, std::string_view prefix)
  {
    if (str.size() < prefix.size())
      return false;

    if (prefix.empty())
      return true;

    return memcmp(str.data(), prefix.data(), prefix.size()) == 0;
  }
}