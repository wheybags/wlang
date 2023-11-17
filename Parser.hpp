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

private:
  template<typename T> T* makeNode();

  struct IntermediateExpressionItem
  {
    #define FOR_EACH_TAGGED_UNION_TYPE(XX) \
      XX(expression, Expression, Expression*) \
      XX(op, Op, Op::Type) \
      XX(callArgs, CallArgs, std::vector<Expression*>)

    #define CLASS_NAME Val
    #include "CreateTaggedUnion.hpp"

    Val val;
    SourceRange source;
  };

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




