#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "Ast.hpp"
#include "Tokeniser.hpp"


class ParseContext;

class AstChunk
{
public:
  AstChunk() = default;
  AstChunk(const AstChunk&) = delete;
  AstChunk(AstChunk&&) noexcept = default;
  ~AstChunk() = default;
  AstChunk& operator=(const AstChunk&) = delete;
  AstChunk& operator=(AstChunk&&) = default;

  template<typename T> T* makeNode();

public:
  Root* root = nullptr;
  std::vector<Type*> createdTypes;
  std::vector<Type*> importedTypes;

private:
  using Node = std::variant<
    std::monostate,
# define X(Type) Type,
    FOR_EACH_AST_TYPE
# undef X
    Scope>;

  using NodeBlock = std::vector<Node>;
  std::vector<NodeBlock> nodeBlocks;
};

class Parser
{
public:
  Parser();

  AstChunk parse(const std::vector<Token>& tokens);

private:
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
  Expression* resolveIntermediateExpression(ParseContext& ctx, IntermediateExpression&& intermediate);
  std::string parseId(ParseContext& ctx);
  int32_t parseInt32(ParseContext& ctx);
  Type* getOrCreateType(ParseContext& ctx, const std::string& typeName);

  #include "ParserRulesDeclarations.inl"
};




