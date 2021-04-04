#include "Parser.hpp"
#include <optional>
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

class ParseTree;

class ParseContext
{
public:
  ParseContext(ParseTree& parser, const std::string* strings, size_t size)
    : parser(parser)
    , next(strings)
    , remaining(size)
  {}

  const std::string& peek(int32_t offset = 0)
  {
    release_assert(has(offset + 1));
    return *next;
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

  bool empty()
  {
    return remaining == 0;
  }

  size_t has(int32_t count)
  {
    return remaining >= count;
  }

public:
  ParseTree& parser;

private:
  const std::string* next = nullptr;
  size_t remaining = 0;

#ifndef NDEBUG
  const std::string* popped = nullptr;
#endif
};

Root* parseRoot(ParseContext& ctx);

const Root* ParseTree::parse(const std::vector<std::string>& tokenStrings)
{
  ParseContext tokens(*this, tokenStrings.data(), tokenStrings.size());
  return parseRoot(tokens);
}

template<typename T> T* ParseTree::makeNode()
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

class ParseFuncs
{
public:

  static FuncList *parseFuncList(ParseContext& ctx)
  {
    FuncList *head = ctx.parser.makeNode<FuncList>();
    head->func = parseFunc(ctx);

    FuncList *previous = head;
    while (!ctx.empty())
    {
      FuncList *funcList = ctx.parser.makeNode<FuncList>();
      funcList->func = parseFunc(ctx);
      previous->next = funcList;
      previous = funcList;
    }

    return head;
  }

  static Func *parseFunc(ParseContext& ctx)
  {
    Func *func = ctx.parser.makeNode<Func>();
    func->returnType = parseReturnType(ctx);
    func->name = parseId(ctx);
    func->argList = parseArgList(ctx);
    func->funcBody = parseFuncBody(ctx);
    return func;
  }

  static ArgList *parseArgList(ParseContext& ctx)
  {
    release_assert(ctx.pop() == KWOpenBracket);
    ArgList *retval = nullptr;

    ArgList *previous = nullptr;
    while (ctx.peek() != KWCloseBracket)
    {
      ArgList *argList = ctx.parser.makeNode<ArgList>();
      argList->arg = parseArg(ctx);

      if (retval == nullptr)
        retval = argList;

      if (previous)
        previous->next = argList;
      previous = argList;
    }
    release_assert(ctx.pop() == KWCloseBracket);

    return retval;
  }

  static Arg *parseArg(ParseContext& ctx)
  {
    Arg *arg = ctx.parser.makeNode<Arg>();
    arg->type = parseType(ctx);
    arg->name = parseId(ctx);
    return arg;
  }

  static FuncBody *parseFuncBody(ParseContext& ctx)
  {
    release_assert(ctx.pop() == KWOpenBrace);
    FuncBody *funcBody = ctx.parser.makeNode<FuncBody>();

    release_assert(ctx.pop() == KWReturn);
    funcBody->retval = parseExpression(ctx);
    release_assert(ctx.pop() == KWSemicolon);

    release_assert(ctx.pop() == KWCloseBrace);
    return funcBody;
  }

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

  static Expression *parseExpression(ParseContext& ctx)
  {
    Expression *expression = ctx.parser.makeNode<Expression>();

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

      CompareEqual *compareEqual = ctx.parser.makeNode<CompareEqual>();
      compareEqual->left = expression;
      compareEqual->right = parseExpression(ctx);

      Expression *compareEqualExpression = ctx.parser.makeNode<Expression>();
      *compareEqualExpression = compareEqual;
      return compareEqualExpression;
    }

    return expression;
  }

  static ReturnType *parseReturnType(ParseContext& ctx)
  {
    ReturnType *returnType = ctx.parser.makeNode<ReturnType>();
    returnType->type = parseType(ctx);
    return returnType;
  }

  static Type *parseType(ParseContext& ctx)
  {
    Type *type = ctx.parser.makeNode<Type>();
    type->id = parseId(ctx);
    return type;
  }

  static Id *parseId(ParseContext& ctx)
  {
    Id *id = ctx.parser.makeNode<Id>();
    id->name = ctx.pop();
    release_assert(!id->name.empty() && id->name[0] == '_' || Str::isAlpha(id->name[0]));
    for (char c : id->name)
      release_assert(c == '_' || Str::isAlphaNumeric(c));
    return id;
  }
};

Root* parseRoot(ParseContext& ctx)
{
  Root* rootNode = ctx.parser.makeNode<Root>();
  rootNode->funcList = ParseFuncs::parseFuncList(ctx);
  return rootNode;
}

