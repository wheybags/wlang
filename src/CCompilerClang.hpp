#pragma once
#include "CCompiler.hpp"

class CCompilerClang : public CCompiler
{
public:
  ~CCompilerClang() override = default;
  void compile(const fs::path& cFilePath, const fs::path& objectFilePath) override;
  void linkExecutable(const std::vector<fs::path>& objects, const fs::path& outputPath) override;

private:
  std::string compilerPath = "/usr/bin/clang";
};