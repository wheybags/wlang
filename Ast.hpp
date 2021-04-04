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

using Node = std::variant<std::monostate,
  Root,
  FuncList,
  Func,
  FuncBody,
  ReturnType,
  Type,
  ArgList,
  Arg,
  Expression,
  Id,
  CompareEqual>;


void dumpJson(const Expression* node, std::string& str, int32_t tabIndex = 0);
void dumpJson(const Root* node, std::string& str, int32_t tabIndex = 0);
void dumpJson(const FuncList* node, std::string& str, int32_t tabIndex = 0);
void dumpJson(const Func* node, std::string& str, int32_t tabIndex = 0);
void dumpJson(const Id* node, std::string& str, int32_t tabIndex = 0);
void dumpJson(const ArgList* node, std::string& str, int32_t tabIndex = 0);
void dumpJson(const Arg* node, std::string& str, int32_t tabIndex = 0);
void dumpJson(const FuncBody* node, std::string& str, int32_t tabIndex = 0);
void dumpJson(const ReturnType* node, std::string& str, int32_t tabIndex = 0);
void dumpJson(const Type* node, std::string& str, int32_t tabIndex = 0);
void dumpJson(const CompareEqual* node, std::string& str, int32_t tabIndex = 0);
