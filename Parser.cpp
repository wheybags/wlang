#include "Parser.hpp"
#include <optional>
#include <unordered_set>
#include "Assert.hpp"

class ParseContext
{
public:
  ParseContext(const Token* tokens, size_t size)
    : next(tokens)
    , remaining(size)
  {}

  const Token& peek()
  {
    release_assert(remaining > 0);
    return *next;
  }

  bool peekCheck(Token::Type type)
  {
    return peek().type == type;
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

  bool popCheck(Token::Type type)
  {
    return pop().type == type;
  }

  bool empty() const
  {
    return remaining == 0;
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

Expression* Parser::resolveIntermediateExpression(IntermediateExpression&& intermediate)
{
  for (Op::Type op = (Op::Type)0; op != Op::Type::ENUM_END; op = (Op::Type)(int32_t(op) + 1))
  {
    for (int32_t i = 1; i < int32_t(intermediate.size()); i += 2)
    {
      if (op == std::get<Op::Type>(intermediate[i]))
      {
        Op* opNode = makeNode<Op>();

        switch (op)
        {
          case Op::Type::CompareEqual:
          case Op::Type::LogicalAnd:
          {
            opNode->left = std::get<Expression*>(intermediate[i-1]);
            opNode->right = std::get<Expression*>(intermediate[i+1]);
            intermediate.erase(intermediate.begin() + i, intermediate.begin() + (i+2));
            break;
          }
          case Op::Type::Call:
          {
            opNode->left = std::get<Expression*>(intermediate[i-1]);
            intermediate.erase(intermediate.begin() + i, intermediate.begin() + (i+1));
            break;
          }
          case Op::Type::ENUM_END:
            release_assert(false);
        }

        opNode->type = op;
        Expression* expression = makeNode<Expression>();
        *expression = opNode;


        intermediate[i-1] = expression;
        i -= 2;
      }
    }
  }

  debug_assert(intermediate.size() == 1);
  return std::get<Expression*>(intermediate[0]);
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

std::string Parser::parseId(ParseContext& ctx)
{
  release_assert(ctx.peek().type == Token::Type::Id);
  return ctx.pop().idValue;
}

int32_t Parser::parseInt32(ParseContext& ctx)
{
  release_assert(ctx.peek().type == Token::Type::Int32);
  return ctx.pop().i32Value;
}

Type* Parser::getType(const std::string& typeName)
{
  auto it = types.find(typeName);

  if (it == types.end())
  {
    Type* type = makeNode<Type>();
    type->name = typeName;
    types.emplace_hint(it, typeName, type);
    return type;
  }
  else
  {
    return it->second;
  }
}

#include "ParserRules.inl"
