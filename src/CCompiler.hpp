#pragma once
#include "Common/Filesystem.hpp"

class CCompiler
{
public:
  virtual ~CCompiler() = default;
  virtual void compile(const fs::path& cFilePath, const fs::path& objectFilePath) = 0;
  virtual void linkExecutable(const std::vector<fs::path>& objects, const fs::path& outputPath) = 0;
};