#include <algorithm>
#include "Grammar.hpp"
#include "TestGrammar.hpp"
#include "ParserGenerator.hpp"
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

const char* wlangGrammarStr = R"STR(
  Root <{Root*}> =
    {{
      Root* rootNode = makeNode<Root>();
      rootNode->rootScope = makeNode<Scope>();
      ctx.pushScope(rootNode->rootScope);
      rootNode->funcList = makeNode<FuncList>();
    }} FuncList<{rootNode->funcList}> $End
    {{
      return rootNode;
    }};


  FuncList <{void}> <{FuncList* funcList}> =
    Func {{funcList->functions.emplace_back(v0);}} FuncList'<{funcList}>;


  FuncList' <{void}> <{FuncList* funcList}> =
    FuncList<{funcList}> | Nil;


  Func <{Func*}> =
    Type $Id
    {{
      Func* func = makeNode<Func>();
      func->returnType = v0;
      func->name = std::move(v1);
    }} "(" Func'<{func}>
    {{ return func; }};


  Func' <{void}> <{Func* func}> =
    ArgList {{ func->argList = v0; }} ")" Block {{ func->funcBody = v1; }}
  |
    ")" Block {{ func->funcBody = v0; }};


  Block <{Block*}> =
    "{" {{ Block* block = makeNode<Block>(); }} StatementList<{block}> "}" {{ return block; }};


  StatementList <{void}> <{Block* block}> =
    Statement {{ block->statements.push_back(v0); }} ";" StatementList'<{block}>;


  StatementList' <{void}> <{Block* block}> =
    StatementList<{block}> | Nil;


  Statement <{Statement*}> =
    {{
      Statement* statement = makeNode<Statement>();
      ReturnStatement* returnStatement = makeNode<ReturnStatement>();
      IntermediateExpression intermediate;
    }}
    "return" Expression<{intermediate}>
    {{
      returnStatement->retval = resolveIntermediateExpression(std::move(intermediate));
      *statement = returnStatement;
      return statement;
    }}
  |
    {{ Statement* statement = makeNode<Statement>(); }}
    $Id Statement'<{v0, statement}>
    {{ return statement; }} ;


  Statement' <{void}> <{const std::string& id, Statement* statement}> =
    // declaration
    $Id
    {{
      VariableDeclaration* variableDeclaration = makeNode<VariableDeclaration>();
      variableDeclaration->type = getType(id);
      variableDeclaration->name = v0;
      *statement = variableDeclaration;
    }}
  |
    // assign
    {{
      Expression* partial = makeNode<Expression>();
      *partial = id;
      IntermediateExpression intermediateL;
      intermediateL.push_back(partial);
    }}
    Expression'<{intermediateL}> "="
    {{
      IntermediateExpression intermediateR;
    }} Expression<{intermediateR}>
    {{
      Assignment* assignment = makeNode<Assignment>();
      assignment->left = resolveIntermediateExpression(std::move(intermediateL));
      assignment->right = resolveIntermediateExpression(std::move(intermediateR));
      *statement = assignment;
    }} ;


  Expression <{void}> <{IntermediateExpression& result}> =
    $Id
    {{
      Expression* expression = makeNode<Expression>();
      *expression = v0;
      result.push_back(expression);
    }} Expression'<{result}>
  |
    $Int32
    {{
      Expression* expression = makeNode<Expression>();
      *expression = v0;
      result.push_back(expression);
    }} Expression'<{result}>;


  Expression' <{void}> <{IntermediateExpression& result}> =
    {{ result.push_back(Op::Type::CompareEqual); }}
    "==" Expression<{result}>
  |
    {{ result.push_back(Op::Type::LogicalAnd); }}
    "&&" Expression<{result}> | Nil;


  Type <{Type*}> =
    $Id {{ return getType(v0); }};


  ArgList <{ArgList*}> =
    Arg
    {{
      ArgList* argList = makeNode<ArgList>();
      argList->arg = v0;
    }}
    ArgList'<{argList}>
    {{ return argList; }};


  ArgList' <{void}> <{ArgList* argList}> =
    "," ArgList {{ argList->next = v0; }}
  |
    Nil;


  Arg <{Arg*}> =
    Type $Id
    {{
      Arg* arg = makeNode<Arg>();
      arg->type = v0;
      arg->name = v1;
      return arg;
    }};
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

std::string readWholeFileAsString(const std::filesystem::path& path)
{
  FILE* f = fopen(path, "rb");
  if (!f)
    return {};

  release_assert(fseek(f, 0, SEEK_END) == 0);
#ifdef WIN32
  int64_t size = _ftelli64(f);
#else
  int64_t size = ftell(f);
#endif

  release_assert(size >= 0);

  release_assert(fseek(f, 0, SEEK_SET) == 0);

  std::string retval;
  retval.resize(size);
  release_assert(fread(retval.data(), 1, size, f) == size);

  fclose(f);

  return retval;
}

void overwriteFileWithString(const std::filesystem::path& path, const std::string_view string)
{
  FILE* f = fopen(path, "wb");
  release_assert(f);
  release_assert(fwrite(string.data(), 1, string.size(), f) == string.size());
  fclose(f);
}

void dumpFirstFollows(Grammar& grammar)
{
  auto pad = [&](const std::string& name)
  {
    int32_t max_len = 0;
    for (const auto& pair: grammar.getRules())
      max_len = std::max(int32_t(pair.first.size()), max_len);

    std::string retval;
    for (int32_t i = 0; i < (max_len - int32_t(name.size())); i++)
      retval += ' ';

    return retval;
  };


  printf("Firsts:\n");
  for (const std::string& name : grammar.getKeys())
  {
    std::string line = "    " + name + " " + pad(name) + "= ";

    for (const std::vector<std::string>& prod_firsts : grammar.first(name))
    {
      for (const std::string& item : prod_firsts)
        line += item + " ";
      line += "| ";
    }

    if (!line.empty())
      line.resize(line.size() - 3);

    puts(line.c_str());
  }

  printf("\nFollows:\n");
  for (const std::string& name : grammar.getKeys())
  {
    std::string line = "    " + name + " " + pad(name) + "= ";

    std::unordered_set<std::string> follows = grammar.follow(name);
    std::vector<std::string> sortedFollows(follows.begin(), follows.end());
    std::sort(sortedFollows.begin(), sortedFollows.end());

    for (const std::string& item : sortedFollows)
      line += item + " ";

    if (!line.empty())
      line.resize(line.size() - 1);

    puts(line.c_str());
  }
}

int main()
{
  test();

  Grammar wlangGrammar(wlangGrammarStr);

  std::filesystem::path rootPath = getPathToThisExecutable();
  while (!std::filesystem::exists(rootPath / "Parser.cpp"))
    rootPath = rootPath.parent_path();

  std::filesystem::path parserImplementationPath = rootPath / "ParserRules.inl";
  std::filesystem::path parserDeclarationPath = rootPath / "ParserRulesDeclarations.inl";

  ParserSource parserSource = generateParser(wlangGrammar);

  overwriteFileWithString(parserImplementationPath, parserSource.implementationSource);
  overwriteFileWithString(parserDeclarationPath, parserSource.declarationSource);

  return 0;
}