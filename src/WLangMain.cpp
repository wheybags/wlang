#include "Tokeniser.hpp"
#include "Parser.hpp"
#include "PlainCGenerator.hpp"
#include "SemanticAnalyser.hpp"
#include "MergedAst.hpp"
#include "Common/Filesystem.hpp"
#include "Process.hpp"

int WLangMain(int argc, char** argv)
{
  fs::path projectRoot;
  if (argc > 1)
    projectRoot = argv[1];
  else
    projectRoot = fs::current_path();

  MergedAst mergedAst;

  auto add = [&](std::string_view path, std::string_view inputString)
  {
    AstChunk* ast = mergedAst.create(path);
    std::vector<Token> tokens = tokenise(inputString);
    parse(*ast, tokens);
    mergedAst.link(ast);
  };

  for (fs::path path : fs::recursive_directory_iterator(projectRoot / "src"))
  {
    if (path.extension() == ".w")
    {
      std::string data;
      release_assert(readWholeFileAsString(path, data));
      add(path.string(), data);
    }
  }

  SemanticAnalyser semanticAnalyser;
  semanticAnalyser.run(mergedAst);

  fs::path buildDirectory = projectRoot / "build_debug";

  std::error_code _;
  fs::create_directories(buildDirectory, _);

  for (const AstChunk* chunk : mergedAst)
  {
    for (const Func* function : chunk->root->funcList->functions)
    {
      PlainCGenerator generator;
      generator.generate(function);
      std::string output = generator.output();
      fs::path outputCFile = buildDirectory / (function->name + ".c");
      fs::path outputObjFile = buildDirectory / (function->name + ".o");
      release_assert(overwriteFileWithString(outputCFile, output));

      fs::path compilerPath = L"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC\\14.36.32532\\bin\\Hostx64\\x64\\cl.exe";

      std::string compilerOutput;
      int32_t exitCode = 0;
      release_assert(runProcess({compilerPath.string(), "/Fo" + outputObjFile.string(), "/c", outputCFile.string()}, compilerOutput, exitCode));
      if (exitCode != 0)
        message_and_abort(compilerOutput.c_str());
    }
  }

  return 0;
}