#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "Ast.hpp"

class ParseContext;

class Parser
{
public:
  Parser();

  const Root* parse(const std::vector<std::string>& tokenStrings);

private:
  template<typename T> T* makeNode();

  Root* parseRoot(ParseContext& ctx);
  FuncList* parseFuncList(ParseContext& ctx);
  Func* parseFunc(ParseContext& ctx);
  ArgList* parseArgList(ParseContext& ctx);
  Arg* parseArg(ParseContext& ctx);
  StatementList* parseStatementList(ParseContext& ctx);
  Statement* parseStatement(ParseContext& ctx);
  Expression *parseExpression(ParseContext& ctx);
  Type *parseType(ParseContext& ctx);
  Id *parseId(ParseContext& ctx);

  Type* getExpressionType(const Expression* expression);

private:
  using Node = std::variant<
    std::monostate,
# define X(Type) Type,
    FOR_EACH_AST_TYPE
# undef X
    Scope>;

  using NodeBlock = std::vector<Node>;
  std::vector<NodeBlock> nodeBlocks;

  std::unordered_map<std::string, Type*> types;
  std::vector<const Assignment*> assignments;
};




