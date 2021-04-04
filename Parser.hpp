#pragma once
#include <vector>
#include <string>
#include "Ast.hpp"

class ParseContext;

class Parser
{
public:
  const Root* parse(const std::vector<std::string>& tokenStrings);
  template<typename T> T* makeNode();

private:
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

private:
  using NodeBlock = std::vector<Node>;
  std::vector<NodeBlock> nodeBlocks;
};




