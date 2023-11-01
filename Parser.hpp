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

  Root* parse(const std::vector<Token>& tokens);
  Type* getType(const std::string& typeName);

public:
  Type* tInt32 = nullptr;
  Type* tBool = nullptr;

private:
  template<typename T> T* makeNode();

  using IntermediateExpressionItem = std::variant<Expression*, Op::Type, std::vector<Expression*>>;
  using IntermediateExpression = std::vector<IntermediateExpressionItem>;
  Expression* resolveIntermediateExpression(IntermediateExpression&& intermediate);
  std::string parseId(ParseContext& ctx);
  int32_t parseInt32(ParseContext& ctx);
  Type* getOrCreateType(const std::string& typeName);

#include "ParserRulesDeclarations.inl"

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




