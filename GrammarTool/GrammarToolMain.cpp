#include <algorithm>
#include "Grammar.hpp"
#include "TestGrammar.hpp"
#include "ParserGenerator.hpp"
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

const char* wlangGrammarStr = R"STR(
  Root                = FuncList $End;
  FuncList            = Func FuncList';
  FuncList'           = FuncList | Nil;
  Func                = Type $Id "(" Func';
  Func'               = ArgList ")" Block | ")" Block;
  Block               = "{" StatementList "}";
  StatementList       = Statement ";" StatementList';
  StatementList'      = StatementList | Nil;
  Statement           = "return" Expression | $Id Statement';

  //                    Decl  Assign
  Statement'          = $Id | Expression' "=" Expression;
  Expression          = $Id Expression' | $Int32 Expression';
  Expression'         = "==" Expression | "&&" Expression | Nil;
  Type                = $Id;
  ArgList             = Arg ArgList';
  ArgList'            = "," ArgList | Nil;
  Arg                 = Type $Id;
)STR";

std::filesystem::path getPathToThisExecutable()
{
#ifdef WIN32
  std::vector<wchar_t> buff;
  buff.resize(8192);
  GetModuleFileNameW(nullptr, buff.data(), DWORD(buff.size()));
  return buff.data();
#else
# error "implement me"
#endif
}

FILE* fopen(const std::filesystem::path& path, const std::string& mode)
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

int main()
{
  test();

  Grammar wlangGrammar(wlangGrammarStr);


//  auto pad = [&](const std::string& name)
//  {
//    int32_t max_len = 0;
//    for (const auto& pair: wlangGrammar.getRules())
//      max_len = std::max(int32_t(pair.first.size()), max_len);
//
//    std::string retval;
//    for (int32_t i = 0; i < (max_len - int32_t(name.size())); i++)
//      retval += ' ';
//
//    return retval;
//  };
//
//
//  printf("Firsts:\n");
//  for (const std::string& name : wlangGrammar.getKeys())
//  {
//    std::string line = "    " + name + " " + pad(name) + "= ";
//
//    for (const std::vector<std::string>& prod_firsts : wlangGrammar.first(name))
//    {
//      for (const std::string& item : prod_firsts)
//        line += item + " ";
//      line += "| ";
//    }
//
//    if (!line.empty())
//      line.resize(line.size() - 3);
//
//    puts(line.c_str());
//  }
//
//  printf("\nFollows:\n");
//  for (const std::string& name : wlangGrammar.getKeys())
//  {
//    std::string line = "    " + name + " " + pad(name) + "= ";
//
//    std::unordered_set<std::string> follows = wlangGrammar.follow(name);
//    std::vector<std::string> sortedFollows(follows.begin(), follows.end());
//    std::sort(sortedFollows.begin(), sortedFollows.end());
//
//    for (const std::string& item : sortedFollows)
//      line += item + " ";
//
//    if (!line.empty())
//      line.resize(line.size() - 1);
//
//    puts(line.c_str());
//  }

  std::filesystem::path rootPath = getPathToThisExecutable();
  while (!std::filesystem::exists(rootPath / "Parser.cpp"))
    rootPath = rootPath.parent_path();

  std::filesystem::path parserTemplatePath = rootPath / "Parser.cpp.template";

  std::string parserSource = generateParser(wlangGrammar);

  FILE* f = fopen(parserTemplatePath, "wb");
  release_assert(f);
  release_assert(fwrite(parserSource.data(), 1, parserSource.size(), f) == parserSource.size());
  fclose(f);

  return 0;
}