#include "Parser.hpp"
#include <optional>
#include <unordered_set>
#include "Assert.hpp"
#include "StringUtil.hpp"

const std::string KWComma = ",";
const std::string KWOpenBracket = "(";
const std::string KWCloseBracket = ")";
const std::string KWOpenBrace = "{";
const std::string KWCloseBrace = "}";
const std::string KWCompareEqual = "==";
const std::string KWReturn = "return";
const std::string KWSemicolon = ";";
const std::string KWAssign = "=";

std::unordered_set<std::string> keywords =
{
  KWComma,
  KWOpenBracket,
  KWCloseBracket,
  KWOpenBrace,
  KWCloseBrace,
  KWCompareEqual,
  KWReturn,
  KWSemicolon,
  KWAssign,
};

class Parser;

class ParseContext
{
public:
  ParseContext(const std::string* strings, size_t size)
    : next(strings)
    , remaining(size)
  {}

  const std::string& peek(int32_t offset = 0)
  {
    release_assert(has(offset + 1));
    return *(next + offset);
  }

  const std::string& pop()
  {
    release_assert(!empty());
    const std::string* retval = next;
    next++;
    remaining--;
#ifndef NDEBUG
    popped = retval;
#endif
    return *retval;
  }

  bool empty() const
  {
    return remaining == 0;
  }

  size_t has(int32_t count) const
  {
    return remaining >= count;
  }

  Scope* getScope() { return scopeStack.back(); }
  void pushScope(Scope* scope) { scopeStack.push_back(scope); }
  void popScope() { scopeStack.pop_back(); }

private:
  std::vector<Scope*> scopeStack;
  const std::string* next = nullptr;
  size_t remaining = 0;

#ifndef NDEBUG
  const std::string* popped = nullptr;
#endif
};

static std::optional<int32_t> parseInt32(std::string_view str)
{
  // TODO: overflow check

  int32_t value = 0;

  for (char c : str)
  {
    if (Str::isNumeric(c))
      value = value * 10 + (int32_t(c) - int32_t('0'));
    else
      return std::nullopt;
  }

  return value;
}

bool isValidId(const std::string& name)
{
  if (name.empty())
    return false;
  if (name[0] != '_' && !Str::isAlpha(name[0]))
    return false;

  for (char c : name)
  {
    if (c != '_' && !Str::isAlphaNumeric(c))
      return false;
  }

  return keywords.find(name) == keywords.end();
}

Parser::Parser()
{
  for (const auto& name: {"i32", "bool"})
  {
    Type* type = makeNode<Type>();
    type->name = name;
    types[name] = type;
  }
}

const Root* Parser::parse(const std::vector<std::string>& tokenStrings)
{
  ParseContext tokens(tokenStrings.data(), tokenStrings.size());
  return parseRoot(tokens);
}

template<typename T> T* Parser::makeNode()
{
  constexpr size_t blockSize = 256;
  if (nodeBlocks.empty() || nodeBlocks.back().size() == blockSize)
  {
    nodeBlocks.resize(nodeBlocks.size() + 1);
    nodeBlocks.back().reserve(blockSize);
  }

  NodeBlock& currentBlock = nodeBlocks.back();
  Node& node = currentBlock.emplace_back(T());
  return &std::get<T>(node);
}

FuncList* Parser::parseFuncList(ParseContext& ctx)
{
  FuncList *head = makeNode<FuncList>();
  head->func = parseFunc(ctx);

  FuncList *previous = head;
  while (!ctx.empty())
  {
    FuncList *funcList = makeNode<FuncList>();
    funcList->func = parseFunc(ctx);
    previous->next = funcList;
    previous = funcList;
  }

  return head;
}

Func* Parser::parseFunc(ParseContext& ctx)
{
  Func *func = makeNode<Func>();
  func->returnType = parseType(ctx);
  func->name = parseId(ctx);

  func->scope = makeNode<Scope>();
  func->scope->parent = ctx.getScope();
  ctx.pushScope(func->scope);

  func->argList = parseArgList(ctx);
  func->funcBody = parseStatementList(ctx);

  ctx.popScope();

  return func;
}

ArgList* Parser::parseArgList(ParseContext& ctx)
{
  release_assert(ctx.pop() == KWOpenBracket);
  ArgList *retval = nullptr;

  ArgList *previous = nullptr;
  while (ctx.peek() != KWCloseBracket)
  {
    ArgList *argList = makeNode<ArgList>();
    argList->arg = parseArg(ctx);

    release_assert(ctx.peek() == KWCloseBracket || ctx.peek() == KWComma);
    if (ctx.peek() == KWComma)
      ctx.pop();

    if (retval == nullptr)
      retval = argList;

    if (previous)
      previous->next = argList;
    previous = argList;
  }
  release_assert(ctx.pop() == KWCloseBracket);

  return retval;
}

Arg* Parser::parseArg(ParseContext& ctx)
{
  Arg *arg = makeNode<Arg>();
  arg->type = parseType(ctx);
  arg->name = parseId(ctx);
  return arg;
}

StatementList* Parser::parseStatementList(ParseContext& ctx)
{
  if(ctx.peek() == KWOpenBrace)
  {
    ctx.pop();

    StatementList* head = nullptr;

    StatementList* previous = nullptr;
    while (ctx.peek() != KWCloseBrace)
    {
      StatementList* statementList = makeNode<StatementList>();

      statementList->statement = parseStatement(ctx);
      release_assert(ctx.pop() == KWSemicolon);

      if (!head)
        head = statementList;

      if (previous)
        previous->next = statementList;

      previous = statementList;
    }
    release_assert(ctx.pop() == KWCloseBrace);

    return head;
  }
  else
  {
    StatementList* statementList = makeNode<StatementList>();
    statementList->statement = parseStatement(ctx);
    release_assert(ctx.pop() == KWSemicolon);
    return statementList;
  }
}

Statement* Parser::parseStatement(ParseContext& ctx)
{
  Statement* statement = makeNode<Statement>();
  if (ctx.peek() == KWReturn)
  {
    ctx.pop();
    ReturnStatement* returnStatement = makeNode<ReturnStatement>();
    returnStatement->retval = parseExpression(ctx);
    *statement = returnStatement;
  }
  else if (ctx.has(2) && isValidId(ctx.peek()) && isValidId(ctx.peek(1)))
  {
    VariableDeclaration* variableDeclaration = makeNode<VariableDeclaration>();
    variableDeclaration->type = parseType(ctx);
    variableDeclaration->name = parseId(ctx);

    auto it = ctx.getScope()->variables.find(variableDeclaration->name->name);
    release_assert(it == ctx.getScope()->variables.end());
    ctx.getScope()->variables.emplace_hint(it, variableDeclaration->name->name, variableDeclaration);

    *statement = variableDeclaration;
  }
  else
  {
    Expression* expression = parseExpression(ctx);

    if (ctx.peek() == KWAssign)
    {
      ctx.pop();

      Assignment* assignment = makeNode<Assignment>();
      assignment->left = expression;
      assignment->right = parseExpression(ctx);
      assignments.push_back(assignment);
      *statement = assignment;
    }
    else
    {
      message_and_abort("invalid statement");
    }
  }

  return statement;
}

Expression* Parser::parseExpression(ParseContext& ctx)
{
  Expression* expression = makeNode<Expression>();

  const std::string &token = ctx.peek();
  std::optional<int32_t> intVal = parseInt32(token);
  if (intVal.has_value())
  {
    *expression = *intVal;
    ctx.pop();
  }
  else
  {
    *expression = parseId(ctx);
  }

  if (ctx.peek() == KWCompareEqual)
  {
    ctx.pop();

    CompareEqual* compareEqual = makeNode<CompareEqual>();
    compareEqual->left = expression;
    compareEqual->right = parseExpression(ctx);

    Expression* compareEqualExpression = makeNode<Expression>();
    *compareEqualExpression = compareEqual;
    return compareEqualExpression;
  }

  return expression;
}

Type* Parser::parseType(ParseContext& ctx)
{
  const std::string& typeName = ctx.pop();
  auto it = types.find(typeName);

  if (it == types.end())
  {
    release_assert(isValidId(typeName));
    Type* type = makeNode<Type>();
    type->name = ctx.pop();
    types.emplace_hint(it, typeName, type);
    return type;
  }
  else
  {
    return it->second;
  }
}

Id* Parser::parseId(ParseContext& ctx)
{
  Id *id = makeNode<Id>();
  id->name = ctx.pop();
  return id;
}

Root* Parser::parseRoot(ParseContext& ctx)
{
  Root* rootNode = makeNode<Root>();
  rootNode->rootScope = makeNode<Scope>();
  ctx.pushScope(rootNode->rootScope);

  rootNode->funcList = parseFuncList(ctx);
  return rootNode;
}
