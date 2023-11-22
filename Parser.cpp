#include "Parser.hpp"
#include <optional>
#include <unordered_set>
#include "Assert.hpp"
#include "BuiltinTypes.hpp"


class Parser
{
public:
  Parser(AstChunk& ast, const Token* tokens, size_t size)
    : ast(ast)
    , next(tokens)
    , remaining(size)
  {}

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
  std::string parseId();
  int32_t parseInt32();

  #include "ParserRulesDeclarations.inl"

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
    popped = retval;
    return *retval;
  }

  const Token& lastPopped()
  {
    release_assert(this->popped);
    return *this->popped;
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

  template<typename T> T* makeNode() { return ast.makeNode<T>(); }

public:
  AstChunk& ast;

  std::vector<Scope*> scopeStack;
  const Token* next = nullptr;
  size_t remaining = 0;
  const Token* popped = nullptr;
};

void parse(AstChunk& ast, const std::vector<Token>& tokens)
{
  Parser parser(ast, tokens.data(), tokens.size());
  ast.root = parser.parseRoot();
}

Expression* Parser::resolveIntermediateExpression(IntermediateExpression&& intermediate)
{
  auto outputOp = [&](Op::Type op, int32_t& i)
  {
    Op* opNode = makeNode<Op>();

    switch (op)
    {
      case Op::Type::Call:
      {
        IntermediateExpressionItem& callable = intermediate[i-1];
        IntermediateExpressionItem& args = intermediate[i+1];

        opNode->left = callable.val.expression();
        opNode->callArgs = std::move(args.val.callArgs());
        intermediate.erase(intermediate.begin() + i, intermediate.begin() + (i+2));

        opNode->type = op;
        Expression* expression = makeNode<Expression>();
        expression->val = opNode;
        expression->source = SourceRange(opNode->left->source.start, args.source.end);

        intermediate[i-1] = IntermediateExpressionItem(expression, expression->source);
        i -= 2;
        break;
      }
      case Op::Type::LogicalNot:
      case Op::Type::UnaryMinus:
      {
        opNode->left = intermediate[i+1].val.expression();
        intermediate.erase(intermediate.begin() + i, intermediate.begin() + (i+1));

        opNode->type = op;
        Expression* expression = makeNode<Expression>();
        expression->val = opNode;
        expression->source = SourceRange(intermediate[i].source.start, opNode->left->source.end);

        intermediate[i] = IntermediateExpressionItem(expression, expression->source);
        break;
      }
      case Op::Type::Add:
      case Op::Type::Subtract:
      case Op::Type::Multiply:
      case Op::Type::Divide:
      case Op::Type::CompareEqual:
      case Op::Type::CompareNotEqual:
      case Op::Type::LogicalAnd:
      case Op::Type::LogicalOr:
      case Op::Type::MemberAccess:
      {
        opNode->left = intermediate[i-1].val.expression();
        opNode->right = intermediate[i+1].val.expression();
        intermediate.erase(intermediate.begin() + i, intermediate.begin() + (i+2));

        opNode->type = op;
        Expression* expression = makeNode<Expression>();
        expression->val = opNode;
        expression->source = SourceRange(opNode->left->source.start, opNode->right->source.end);

        intermediate[i-1] = IntermediateExpressionItem(expression, expression->source);
        i -= 2;
        break;
      }

      case Op::Type::ENUM_END:
        release_assert(false);
    }
  };

#ifndef NDEBUG
  for (int32_t i = 0; i < int32_t(intermediate.size()); i++)
  {
    release_assert(intermediate[i].source.start > SourceLocation());
    release_assert(intermediate[i].source.end > intermediate[i].source.start);
  }
#endif

  // See https://en.cppreference.com/w/c/language/operator_precedence

  // group 1
  for (int32_t i = 0; i < int32_t(intermediate.size()); i++)
  {
    if (!intermediate[i].val.isOp())
      continue;
    Op::Type op = intermediate[i].val.op();
    if (op == Op::Type::Call || op == Op::Type::MemberAccess)
      outputOp(op, i);
  }

  // group 2
  for (int32_t i = int32_t(intermediate.size()) - 1; i >= 0; i--)
  {
    if (!intermediate[i].val.isOp())
      continue;
    Op::Type op = intermediate[i].val.op();
    if (op == Op::Type::LogicalNot || op == Op::Type::UnaryMinus)
      outputOp(op, i);
  }

  // group 3
  for (int32_t i = 0; i < int32_t(intermediate.size()); i++)
  {
    if (!intermediate[i].val.isOp())
      continue;
    Op::Type op = intermediate[i].val.op();
    if (op == Op::Type::Multiply || op == Op::Type::Divide)
      outputOp(op, i);
  }

  // group 4
  for (int32_t i = 0; i < int32_t(intermediate.size()); i++)
  {
    if (!intermediate[i].val.isOp())
      continue;
    Op::Type op = intermediate[i].val.op();
    if (op == Op::Type::Add || op == Op::Type::Subtract)
      outputOp(op, i);
  }

  // group 7
  for (int32_t i = 0; i < int32_t(intermediate.size()); i++)
  {
    if (!intermediate[i].val.isOp())
      continue;
    Op::Type op = intermediate[i].val.op();
    if (op == Op::Type::CompareEqual || op == Op::Type::CompareNotEqual)
      outputOp(op, i);
  }

  // group 11
  for (int32_t i = 0; i < int32_t(intermediate.size()); i++)
  {
    if (!intermediate[i].val.isOp())
      continue;
    Op::Type op = intermediate[i].val.op();
    if (op == Op::Type::LogicalAnd)
      outputOp(op, i);
  }

  // group 12
  for (int32_t i = 0; i < int32_t(intermediate.size()); i++)
  {
    if (!intermediate[i].val.isOp())
      continue;
    Op::Type op = intermediate[i].val.op();
    if (op == Op::Type::LogicalOr)
      outputOp(op, i);
  }

  debug_assert(intermediate.size() == 1);
  return intermediate[0].val.expression();
}

std::string Parser::parseId()
{
  release_assert(peek().type == Token::Type::Id);
  return pop().idValue;
}

int32_t Parser::parseInt32()
{
  release_assert(peek().type == Token::Type::Int32);
  return pop().i32Value;
}

#include "ParserRules.inl"
