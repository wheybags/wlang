#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "Ast.hpp"
#include "Tokeniser.hpp"


class ParseContext;

class Parser
{
public:
  Parser();

  const Root* parse(const std::vector<Token>& tokens);

private:
  template<typename T> T* makeNode();

  Root* parseRoot(ParseContext& ctx);
  FuncList* parseFuncList(ParseContext& ctx);
  void parseFuncListP(FuncList* funcList, ParseContext& ctx);
  Func* parseFunc(ParseContext& ctx);
  void parseFuncP(Func* func, ParseContext& ctx);
  Block* parseBlock(ParseContext& ctx);
  void parseStatementList(Block* block, ParseContext& ctx);
  void parseStatementListP(Block* block, ParseContext& ctx);
  Statement* parseStatement(ParseContext& ctx);
  void parseStatementP(Id* id, Statement* statement, ParseContext& ctx);
  Type* parseType(ParseContext& ctx, Id* fromId = nullptr);
  ArgList* parseArgList(ParseContext& ctx);
  void parseArgListP(ArgList* argList, ParseContext& ctx);
  Arg* parseArg(ParseContext& ctx);
  Expression* parseExpression(ParseContext& ctx);
  Expression* parseExpressionP(Expression* partial, ParseContext& ctx);
  Id* parseId(ParseContext& ctx);

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




