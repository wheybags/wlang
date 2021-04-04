#pragma once
#include <string>
#include <cstdint>
#include <variant>

/*
  Root          = FuncList
  FuncList      = Func
  FuncList      = FuncList Func
  Func          = ReturnType Id "(" ArgList ")" FuncBody
  Func          = ReturnType Id "(" ")"         FuncBody
  FuncBody      = "{" "return" Expression ";" "}"
  ReturnType    = "void"
  ReturnType    = Type
  Type          = Id
  ArgList       = Arg
  ArgList       = Arg "," ArgList
  Arg           = Type Id
  Expression    = Id
  Expression    = Number
  Expression    = CompareEquals
  CompareEquals = Expression "==" Expression
 */

struct Root;
struct FuncList;
struct Func;
struct FuncBody;
struct ReturnType;
struct Type;
struct ArgList;
struct Arg;
struct Id;
struct CompareEqual;

using Expression = std::variant<std::monostate,
  Id*,
  int32_t,
  CompareEqual*
>;

struct Root
{
  FuncList* funcList = nullptr;
};

struct FuncList
{
  Func* func = nullptr;
  FuncList* next = nullptr;
};

struct Func
{
  ReturnType* returnType = nullptr;
  Id* name = nullptr;
  ArgList* argList = nullptr;
  FuncBody* funcBody = nullptr;
};

struct Id
{
  std::string name;
};

struct ArgList
{
  Arg* arg = nullptr;
  ArgList* next = nullptr;
};

struct Arg
{
  Type* type = nullptr;
  Id* name = nullptr;
};

struct FuncBody
{
  Expression* retval = nullptr;
};

struct ReturnType
{
  Type* type = nullptr;
};

struct Type
{
  Id* id = nullptr;
};

struct CompareEqual
{
  Expression* left = nullptr;
  Expression* right = nullptr;
};

#define FOR_EACH_AST_TYPE \
  X(Root) \
  X(FuncList) \
  X(Func) \
  X(FuncBody) \
  X(ReturnType) \
  X(Type) \
  X(ArgList) \
  X(Arg) \
  X(Expression) \
  X(Id) \
  X(CompareEqual)


using Node = std::variant<
# define X(Type) Type,
  FOR_EACH_AST_TYPE
# undef X
  std::monostate>;

# define X(Type) void dumpJson(const Type* node, std::string& str, int32_t tabIndex = 0);
FOR_EACH_AST_TYPE
# undef X
