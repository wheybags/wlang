#include "Filesystem.hpp"
#include "Assert.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

fs::path getPathToThisExecutable()
{
#ifdef WIN32
  std::vector<wchar_t> buff;
  buff.resize(8192);
  GetModuleFileNameW(nullptr, buff.data(), DWORD(buff.size()));
  return buff.data();
#elif __APPLE__
  std::string temp;
  temp.resize(4096);

  while (true)
  {
    uint32_t bufSize = uint32_t(temp.size());
    if (_NSGetExecutablePath(temp.data(), &bufSize) == 0)
      break;
    temp.resize(std::max(bufSize, uint32_t(temp.size()) * 2U));
  }

  temp.resize(strlen(temp.data()));

  return temp;
#endif
}

FILE* fopen(const fs::path& path, const std::string& mode)
{
#ifdef WIN32
  std::wstring wideMode;
  for (char c: mode)
    wideMode.push_back(c);

  return _wfopen(path.wstring().c_str(), wideMode.c_str());
#else
  return fopen(path.c_str(), mode.c_str());
#endif
}

[[nodiscard]] bool readWholeFileAsString(const fs::path& path, std::string& string)
{
  FILE* f = fopen(path, "rb");
  if (!f)
    return false;

  if (fseek(f, 0, SEEK_END) != 0)
    return false;

#ifdef WIN32
  int64_t size = _ftelli64(f);
#else
  int64_t size = ftell(f);
#endif

  if (size == -1)
    return false;

  if (size == 0)
    return true;

  if (fseek(f, 0, SEEK_SET) != 0)
    return false;

  string.resize(size);
  if (int64_t(fread(string.data(), 1, size, f)) != size)
    return false;

  if (fclose(f) != 0)
    return false;

  return true;
}

bool overwriteFileWithString(const fs::path& path, std::string_view string)
{
  FILE* f = fopen(path, "wb");
  if (!f)
    return false;
  if (fwrite(string.data(), 1, string.size(), f) != string.size())
    return false;
  if (fclose(f) != 0)
    return false;

  return true;
}