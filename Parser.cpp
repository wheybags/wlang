#include "Parser.hpp"
#include <optional>
#include <unordered_set>
#include "Assert.hpp"
#include "StringUtil.hpp"



class Parser;

class ParseContext
{
public:
  ParseContext(const Token* tokens, size_t size)
    : next(tokens)
    , remaining(size)
  {}

  const Token& peek(int32_t offset = 0)
  {
    release_assert(has(offset + 1));
    return *(next + offset);
  }

  const Token& pop()
  {
    release_assert(!empty());
    const Token* retval = next;
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
  const Token* next = nullptr;
  size_t remaining = 0;

#ifndef NDEBUG
  const Token* popped = nullptr;
#endif
};

Parser::Parser()
{
  for (const auto& name: {"i32", "bool"})
  {
    Type* type = makeNode<Type>();
    type->name = name;
    types[name] = type;
  }
}

const Root* Parser::parse(const std::vector<Token>& tokenStrings)
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
  release_assert(ctx.pop().type == Token::Type::OpenBracket);
  ArgList *retval = nullptr;

  ArgList *previous = nullptr;
  while (ctx.peek().type != Token::Type::CloseBracket)
  {
    ArgList *argList = makeNode<ArgList>();
    argList->arg = parseArg(ctx);

    release_assert(ctx.peek().type == Token::Type::CloseBracket || ctx.peek().type == Token::Type::Comma);
    if (ctx.peek().type == Token::Type::Comma)
      ctx.pop();

    if (retval == nullptr)
      retval = argList;

    if (previous)
      previous->next = argList;
    previous = argList;
  }
  release_assert(ctx.pop().type == Token::Type::CloseBracket);

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
  if(ctx.peek().type == Token::Type::OpenBrace)
  {
    ctx.pop();

    StatementList* head = nullptr;

    StatementList* previous = nullptr;
    while (ctx.peek().type != Token::Type::CloseBrace)
    {
      StatementList* statementList = makeNode<StatementList>();

      statementList->statement = parseStatement(ctx);
      release_assert(ctx.pop().type == Token::Type::Semicolon);

      if (!head)
        head = statementList;

      if (previous)
        previous->next = statementList;

      previous = statementList;
    }
    release_assert(ctx.pop().type == Token::Type::CloseBrace);

    return head;
  }
  else
  {
    StatementList* statementList = makeNode<StatementList>();
    statementList->statement = parseStatement(ctx);
    release_assert(ctx.pop().type == Token::Type::Semicolon);
    return statementList;
  }
}

Statement* Parser::parseStatement(ParseContext& ctx)
{
  Statement* statement = makeNode<Statement>();
  if (ctx.peek().type == Token::Type::Return)
  {
    ctx.pop();
    ReturnStatement* returnStatement = makeNode<ReturnStatement>();
    returnStatement->retval = parseExpression(ctx);
    *statement = returnStatement;
  }
  else if (ctx.has(2) && ctx.peek().type == Token::Type::Id && ctx.peek(1).type == Token::Type::Id)
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

    if (ctx.peek().type == Token::Type::Assign)
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

  const Token &token = ctx.peek();
  if (token.type == Token::Type::Int32)
  {
    *expression = token.i32Value;
    ctx.pop();
  }
  else
  {
    *expression = parseId(ctx);
  }

  if (ctx.peek().type == Token::Type::CompareEqual)
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
  const Token& typeName = ctx.pop();
  release_assert(typeName.type == Token::Type::Id);

  auto it = types.find(typeName.idValue);

  if (it == types.end())
  {
    Type* type = makeNode<Type>();
    type->name = typeName.idValue;
    types.emplace_hint(it, typeName.idValue, type);
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
  release_assert(ctx.peek().type == Token::Type::Id);
  id->name = ctx.pop().idValue;
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
