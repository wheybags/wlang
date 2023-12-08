
int WLangMain(int argc, char** argv);

#if WIN32
#include <string>
#include <vector>
#include "Common/StringUtil.hpp"

int wmain(int argc, wchar_t** argv)
{
  std::vector<std::string> newArgStrs;
  for (int i = 0; i < argc; i++)
    newArgStrs.emplace_back(Str::wstringToUtf8(argv[i]));

  std::vector<char*> newArgvArray;
  for (int i = 0; i < argc; i++)
    newArgvArray.emplace_back(newArgStrs[i].data());
  newArgvArray.emplace_back(nullptr);

  return WLangMain(argc, newArgvArray.data());
}
#else
int main(int argc, char** argv) { return WLangMain(argc, argv); }
#endif
