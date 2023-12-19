#include "CCompilerClang.hpp"
#include "Process.hpp"
#include "Common/Assert.hpp"

void CCompilerClang::compile(const fs::path& cFilePath, const fs::path& objectFilePath)
{
  std::string output;
  int32_t exitCode = 0;
  release_assert(runProcess({this->compilerPath, "-c", cFilePath.string(), "-o", objectFilePath}, output, exitCode));
  if (exitCode != 0)
    message_and_abort(output.c_str());
}

void CCompilerClang::linkExecutable(const std::vector<fs::path>& objects, const fs::path& outputPath)
{
  std::vector<std::string> command;
  command.reserve(objects.size() + 3);
  command.emplace_back(this->compilerPath);
  for (const fs::path& object : objects)
    command.emplace_back(object.string());
  command.emplace_back("-o");
  command.emplace_back(outputPath);

  std::string output;
  int32_t exitCode = 0;
  release_assert(runProcess(command, output, exitCode));
  if (exitCode != 0)
    message_and_abort(output.c_str());
}
