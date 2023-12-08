#include "StringUtil.hpp"
#include <cstring>

#ifdef WIN32
#include <windows.h>
#endif

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
      return false;

    return memcmp(str.data(), prefix.data(), prefix.size()) == 0;
  }

  bool endsWith(std::string_view str, std::string_view suffix)
  {
    if (str.size() < suffix.size())
      return false;

    if (suffix.empty())
      return false;

    return memcmp(str.data() + str.size() - suffix.size(), suffix.data(), suffix.size()) == 0;
  }

#ifdef WIN32
  std::string wstringToUtf8(std::wstring_view wstr)
  {
    if (wstr.empty())
      return {};

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), int(wstr.length()), nullptr, 0, nullptr, nullptr);

    std::string retval(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), int(wstr.length()), &retval[0], sizeNeeded, nullptr, nullptr);
    return retval;
  }

  std::wstring utf8ToWstring(std::string_view utf8)
  {
    if (utf8.empty())
      return {};

    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), int(utf8.length()), nullptr, 0);

    std::wstring retval(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), int(utf8.length()), &retval[0], sizeNeeded);
    return retval;
  }
#endif
}