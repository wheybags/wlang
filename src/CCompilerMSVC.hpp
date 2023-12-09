#pragma once
#ifdef WIN32
#include "CCompiler.hpp"

class CCompilerMSVC : public CCompiler
{
public:
  CCompilerMSVC();
  virtual ~CCompilerMSVC() = default;

  void compile(const fs::path& cFilePath, const fs::path& objectFilePath) override;
  void linkExecutable(const std::vector<fs::path>& objects, const fs::path& outputPath) override;

private:
  fs::path compilerPath;
  fs::path linkerPath;
  std::wstring evironmentBlock;
};
#endif