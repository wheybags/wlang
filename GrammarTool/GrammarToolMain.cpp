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
      Root* rootNode = ctx.makeNode<Root>();
      rootNode->funcList = ctx.makeNode<FuncList>();

      rootNode->funcList->scope = ctx.makeNode<Scope>();
      ctx.pushScope(rootNode->funcList->scope);

    }} FuncList<{rootNode->funcList}> $End
    {{
      ctx.popScope();
      return rootNode;
    }};


  FuncList <{void}> <{FuncList* funcList}>  =
    Func
    {{
      funcList->functions.emplace_back(v0);
      funcList->scope->variables.insert_or_assign(v0->name, v0);
    }}
    FuncList'<{funcList}>
  |
    "class" BaseType
    {{
      Class* newClass = ctx.makeNode<Class>();
      // TODO: assert that class name is not a builtin type
      release_assert(!v0->typeClass);
      v0->typeClass = newClass;
      newClass->type = v0;
      funcList->classes.emplace_back(newClass);
    }}
    "{" ClassMemberList<{newClass}> "}"
    FuncList'<{funcList}>
  ;

  ClassMemberList <{void}> <{Class* newClass}> =
    Type $Id TheRestOfADeclaration
    {{
      VariableDeclaration* variableDeclaration = ctx.makeNode<VariableDeclaration>();
      variableDeclaration->type = v0;
      variableDeclaration->name = v1;
      variableDeclaration->initialiser = v2;
      newClass->memberVariableOrder.emplace_back(variableDeclaration->name);
      newClass->memberVariables.insert_or_assign(variableDeclaration->name, variableDeclaration);
    }}
    ";" ClassMemberList<{newClass}>
  |
    Nil
  ;

  FuncList' <{void}> <{FuncList* funcList}> =
    FuncList<{funcList}> | Nil;


  Func <{Func*}> =
    Type $Id
    {{
      Func* func = ctx.makeNode<Func>();
      func->argsScope = ctx.makeNode<Scope>();
      func->returnType = v0;
      func->name = std::move(v1);
    }}
    "(" ArgList<{func}> ")" Block
    {{
      func->funcBody = v2;
      func->funcBody->scope->parent2 = func->argsScope;
      return func;
    }}
  ;

  Block <{Block*}> =
    "{"
    {{
      Block* block = ctx.makeNode<Block>();
      block->scope = ctx.makeNode<Scope>();
      block->scope->parent = ctx.getScope();
      ctx.pushScope(block->scope);
    }}
    StatementList<{block}> "}"
    {{
      ctx.popScope();
      return block;
    }}
  ;


  StatementList <{void}> <{Block* block}> =
    Statement {{ block->statements.push_back(v0); }} StatementList'<{block}>;


  StatementList' <{void}> <{Block* block}> =
    StatementList<{block}> | Nil;


  Statement <{Statement*}>
    {{ Statement* statement = ctx.makeNode<Statement>(); }}
  =
    {{
      ReturnStatement* returnStatement = ctx.makeNode<ReturnStatement>();
      IntermediateExpression intermediate;
    }}
    "return" Expression<{intermediate}> ";"
    {{
      returnStatement->retval = resolveIntermediateExpression(ctx, std::move(intermediate));
      *statement = returnStatement;
    }}
  |
    {{ IfElseChain* ifElseChain = ctx.makeNode<IfElseChain>(); }}
    "if" TheRestOfAnIf<{*ifElseChain}>
    {{ *statement = ifElseChain; }}
  |
    // declaration, or an expression that starts with $Id
    // This awkwardness exists because declarations (and assignments) are not expressions, which is a design choice
    // We basically need to copy paste the rules from Expression into Statement, so we can allow any expression as a statement
    {{ SourceRange idSource = ctx.peek().source; }}
    $Id StatementThatStartsWithId <{v0, idSource, statement}> ";"
  |
    // statement that starts with an expression that starts with $Int32
    $Int32
    {{
      Expression* partial = ctx.makeNode<Expression>();
      partial->val = v0;
      partial->source = ctx.lastPopped().source;
      IntermediateExpression intermediate;
      intermediate.emplace_back(partial, partial->source);
    }}
    Expression'<{intermediate}> TheRestOfAStatement<{std::move(intermediate), statement}> ";"
  |
    // statement that starts with an expression that starts with logical not (why would you want this? dunno)
    "!"
    {{
      IntermediateExpression intermediate;
      intermediate.emplace_back(Op::Type::LogicalNot, ctx.lastPopped().source);
    }}
    Expression<{intermediate}> TheRestOfAStatement<{std::move(intermediate), statement}> ";"
  |
    // statement that starts with an expression that starts with unary minus (again, why would you want this?)
    "-"
    {{
      IntermediateExpression intermediate;
      intermediate.emplace_back(Op::Type::UnaryMinus, ctx.lastPopped().source);
    }}
    Expression<{intermediate}> TheRestOfAStatement<{std::move(intermediate), statement}> ";"
  |
    // statement that starts with an expression that starts with false (again, why would you want this?)
    "false"
    {{
      Expression* partial = ctx.makeNode<Expression>();
      partial->val = false;
      partial->source = ctx.lastPopped().source;
      IntermediateExpression intermediate;
      intermediate.emplace_back(partial, partial->source);
    }}
    Expression'<{intermediate}> TheRestOfAStatement<{std::move(intermediate), statement}> ";"
  |
    // statement that starts with an expression that starts with true (again, why would you want this?)
    "true"
    {{
      Expression* partial = ctx.makeNode<Expression>();
      partial->val = true;
      partial->source = ctx.lastPopped().source;
      IntermediateExpression intermediate;
      intermediate.emplace_back(partial, partial->source);
    }}
    Expression'<{intermediate}> TheRestOfAStatement<{std::move(intermediate), statement}> ";"
  ;
    {{ return statement; }}


  TheRestOfAnIf <{void}> <{IfElseChain& chain}>
  =
    {{ IntermediateExpression intermediate; }}
    "(" Expression<{intermediate}> ")" Block
    {{
      IfElseChainItem* item = ctx.makeNode<IfElseChainItem>();
      item->condition = resolveIntermediateExpression(ctx, std::move(intermediate));
      item->block = v0;
      chain.items.emplace_back(item);
    }}
    TheRestOfAnIf'<{chain}>
  ;


  TheRestOfAnIf' <{void}> <{IfElseChain& chain}>
  =
    "else" TheRestOfAnIf''<{chain}>
  |
    Nil
  ;


  TheRestOfAnIf'' <{void}> <{IfElseChain& chain}>
  =
    "if" TheRestOfAnIf<{chain}>
  |
    Block
    {{
      IfElseChainItem* item = ctx.makeNode<IfElseChainItem>();
      item->block = v0;
      chain.items.emplace_back(item);
    }}
  ;


  StatementThatStartsWithId <{void}> <{const std::string& id, SourceRange idSource, Statement* statement}> =
    // declaration
    {{
      VariableDeclaration* variableDeclaration = ctx.makeNode<VariableDeclaration>();
      variableDeclaration->type = TypeRef { .type = getOrCreateType(ctx, id) };
    }}
    Type'<{variableDeclaration->type}>
    $Id TheRestOfADeclaration
    {{
      SourceRange declarationEnd = ctx.lastPopped().source;

      variableDeclaration->name = v0;
      variableDeclaration->initialiser = v1;
      variableDeclaration->source = SourceRange(idSource.start, declarationEnd.end);

      ctx.getScope()->variables.insert_or_assign(variableDeclaration->name, variableDeclaration);
      *statement = variableDeclaration;
    }}
  |
    // assign, or standalone expression that starts with id
    {{
      Expression* partial = ctx.makeNode<Expression>();
      partial->val = id;
      partial->source = idSource;

      IntermediateExpression intermediate;
      intermediate.emplace_back(partial, partial->source);
    }}
    // The statement A*b is ambiguous - is it a multiplication or pointer declaration?
    // Here we resolve the ambiguity - it's a pointer declaration. We do this by using a special version
    // of the Expression' rule that has no multiply operator.
    // Maybe one day I will add a fancy selection based on whether A exists as a variable, but TBH it's a kinda
    // useless construct so I probably won't.
    Expression'NoMul<{intermediate}>
    TheRestOfAStatement<{std::move(intermediate), statement}>
  |
    Nil
    {{
      Expression* expression = ctx.makeNode<Expression>();
      expression->val = id;
      *statement = expression;
    }}
  ;


  TheRestOfADeclaration <{Expression*}> =
    {{ IntermediateExpression intermediateExpression; }}
    "=" Expression<{intermediateExpression}>
    {{ return resolveIntermediateExpression(ctx, std::move(intermediateExpression)); }}
  |
    Nil
    {{ return nullptr; }}
  ;


  TheRestOfAStatement <{void}> <{IntermediateExpression intermediateExpression, Statement* statement}> =
    "="
    {{
      IntermediateExpression intermediateR;
    }}
    Expression<{intermediateR}>
    {{
      Assignment* assignment = ctx.makeNode<Assignment>();
      assignment->left = resolveIntermediateExpression(ctx, std::move(intermediateExpression));
      assignment->right = resolveIntermediateExpression(ctx, std::move(intermediateR));
      *statement = assignment;
    }}
  |
    Nil
    {{ *statement = resolveIntermediateExpression(ctx, std::move(intermediateExpression)); }};


  Expression <{void}> <{IntermediateExpression& result}>
     {{ SourceRange source = ctx.peek().source; }}
  =
    $Id
    {{
      Expression* expression = ctx.makeNode<Expression>();
      expression->val = v0;
      expression->source = source;
      result.emplace_back(expression, expression->source);
    }} Expression'<{result}>
  |
    $Int32
    {{
      Expression* expression = ctx.makeNode<Expression>();
      expression->val = v0;
      expression->source = source;
      result.emplace_back(expression, expression->source);
    }} Expression'<{result}>
  |
    "false"
    {{
      Expression* expression = ctx.makeNode<Expression>();
      expression->val = false;
      expression->source = source;
      result.emplace_back(expression, expression->source);
    }} Expression'<{result}>
  |
    "true"
    {{
      Expression* expression = ctx.makeNode<Expression>();
      expression->val = true;
      expression->source = source;
      result.emplace_back(expression, expression->source);
    }} Expression'<{result}>
  |
    "!"
    {{
      result.emplace_back(Op::Type::LogicalNot, source);
    }} Expression<{result}>
  |
    "-"
    {{
      result.emplace_back(Op::Type::UnaryMinus, source);
    }} Expression<{result}>
  ;


  Expression'NoMul <{void}> <{IntermediateExpression& result}> =
    {{ result.emplace_back(Op::Type::CompareEqual, ctx.peek().source); }}
    "==" Expression<{result}>
  |
    {{ result.emplace_back(Op::Type::CompareNotEqual, ctx.peek().source); }}
    "!=" Expression<{result}>
  |
    {{ result.emplace_back(Op::Type::LogicalAnd, ctx.peek().source); }}
    "&&" Expression<{result}>
  |
    {{ result.emplace_back(Op::Type::LogicalOr, ctx.peek().source); }}
    "||" Expression<{result}>
  |
    {{ result.emplace_back(Op::Type::Add, ctx.peek().source); }}
    "+" Expression<{result}>
  |
    {{ result.emplace_back(Op::Type::Subtract, ctx.peek().source); }}
    "-" Expression<{result}>
  |
    {{ result.emplace_back(Op::Type::Divide, ctx.peek().source); }}
    "/" Expression<{result}>
  |
    {{ result.emplace_back(Op::Type::MemberAccess, ctx.peek().source); }}
    "." Expression<{result}>
  |
    {{
      result.emplace_back(Op::Type::Call, ctx.peek().source);
      std::vector<Expression*> argList;
      SourceRange openBracket = ctx.peek().source;
     }}
    "("
     CallParamList<{argList}>
    {{
      SourceRange closeBracket = ctx.peek().source;
      result.emplace_back(std::move(argList), SourceRange(openBracket.start, closeBracket.end));
    }}
    ")"
  |
    Nil;


  Expression' <{void}> <{IntermediateExpression& result}> =
    {{ result.emplace_back(Op::Type::Multiply, ctx.peek().source); }}
    "*" Expression<{result}>
  |
    Expression'NoMul<{result}>
  ;

  CallParamList <{void}> <{std::vector<Expression*>& argList}> =
    {{ IntermediateExpression intermediateExpression; }}
    Expression<{intermediateExpression}>
    {{ argList.emplace_back(resolveIntermediateExpression(ctx, std::move(intermediateExpression))); }}
    CallParamList'<{argList}>
  |
    Nil;

  CallParamList' <{void}> <{std::vector<Expression*>& argList}> =
    "," CallParamList<{argList}>
  |
    Nil;

  Type <{TypeRef}> =
    BaseType
    {{ TypeRef retval{ .type = v0 }; }}
    Type'<{retval}>
    {{ return retval; }}
  ;

  Type' <{void}> <{TypeRef& typeRef}> =
    "*" {{ typeRef.pointerDepth++; }} Type'<{typeRef}>
  |
    Nil;

  BaseType <{Type*}> =
    $Id {{ return getOrCreateType(ctx, v0); }};

  ArgList <{void}> <{Func* func}> =
    Arg
    {{
      func->argsOrder.emplace_back(v0->name);
      func->argsScope->variables.insert_or_assign(v0->name, v0);
    }}
    ArgList'<{func}>
  |
    Nil
  ;

  ArgList' <{void}> <{Func* func}> =
    "," ArgList<{func}>
  |
    Nil
  ;

  Arg <{VariableDeclaration*}> =
    Type $Id
    {{
      VariableDeclaration* arg = ctx.makeNode<VariableDeclaration>();
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
  release_assert(int64_t(fread(retval.data(), 1, size, f)) == size);

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

  dumpFirstFollows(wlangGrammar);

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