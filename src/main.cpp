
int WLangMain(int argc, char** argv);

#if WIN32
#include <windows.h>
#include <stringapiset.h>
#include <string>
#include <vector>

std::string wstringToUtf8(std::wstring_view wstr)
{
  if (wstr.empty())
    return {};

  int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), int(wstr.length()), nullptr, 0, nullptr, nullptr);

  std::string retval(sizeNeeded, 0);
  WideCharToMultiByte(CP_UTF8, 0, wstr.data(), int(wstr.length()), &retval[0], sizeNeeded, nullptr, nullptr);
  return retval;
}

int wmain(int argc, wchar_t** argv)
{
  std::vector<std::string> newArgStrs;
  for (int i = 0; i < argc; i++)
    newArgStrs.emplace_back(wstringToUtf8(argv[i]));

  std::vector<char*> newArgvArray;
  for (int i = 0; i < argc; i++)
    newArgvArray.emplace_back(newArgStrs[i].data());
  newArgvArray.emplace_back(nullptr);

  return WLangMain(argc, newArgvArray.data());
}
#else
int main(int argc, char** argv) { return WLangMain(argc, argv); }
#endif
