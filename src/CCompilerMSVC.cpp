#ifdef WIN32

#include "CCompilerMSVC.hpp"
#include "Process.hpp"
#include "Common/Assert.hpp"
#include "windows.h"

CCompilerMSVC::CCompilerMSVC()
{
  this->compilerPath = L"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.36.32532\\bin\\Hostx64\\x64\\cl.exe";
  this->linkerPath = L"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.36.32532\\bin\\Hostx64\\x64\\link.exe";

  wchar_t* baseEnvironment = GetEnvironmentStringsW();

  std::wstring libPart;

  wchar_t* current = baseEnvironment;
  while (*current)
  {
    std::wstring_view var = std::wstring_view(current);
    if (var.starts_with(L"LIB="))
      libPart = var;
    else
      this->evironmentBlock += var;

    this->evironmentBlock += L'\0';
    current += var.length() + 1;
  }

  if (libPart.empty())
    libPart = L"LIB=";
  else if (libPart != L"LIB=")
    libPart += L';';

  libPart += L"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.36.32532\\ATLMFC\\lib\\x64;C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.36.32532\\lib\\x64;C:\\Program Files (x86)\\Windows Kits\\NETFXSDK\\4.8\\lib\\um\\x64;C:\\Program Files (x86)\\Windows Kits\\10\\lib\\10.0.22621.0\\ucrt\\x64;C:\\Program Files (x86)\\Windows Kits\\10\\\\lib\\10.0.22621.0\\\\um\\x64";

  this->evironmentBlock += libPart;
  this->evironmentBlock += L'\0';

  this->evironmentBlock += L'\0';

  FreeEnvironmentStringsW(baseEnvironment);
}

void CCompilerMSVC::compile(const fs::path& cFilePath, const fs::path& objectFilePath)
{
  std::string output;
  int32_t exitCode = 0;
  release_assert(runProcess({compilerPath.string(), "/Fo" + objectFilePath.string(), "/c", cFilePath.string()}, output, exitCode));
  if (exitCode != 0)
    message_and_abort(output.c_str());
}

void CCompilerMSVC::linkExecutable(const std::vector<fs::path>& objects, const fs::path& outputPath)
{
  std::string output;
  int32_t exitCode = 0;

  std::vector<std::string> args = {linkerPath.string(), "/OUT:" + outputPath.string()};
  for (const fs::path& item : objects)
    args.emplace_back(item.string());

  release_assert(runProcess(args, output, exitCode, this->evironmentBlock.data()));
  if (exitCode != 0)
    message_and_abort(output.c_str());
}

#endif