#pragma once
#include <string>
#include <cstdint>
#include <variant>
#include <vector>
#include <unordered_map>

struct Root;
struct FuncList;
struct Func;
struct Block;
struct VariableDeclaration;
struct Assignment;
struct ReturnStatement;
struct Type;
struct ArgList;
struct Arg;
struct Id;
struct CompareEqual;

struct Scope;

using Expression = std::variant<std::monostate,
  Id*,
  int32_t,
  CompareEqual*
>;

using Statement = std::variant<std::monostate,
  ReturnStatement*,
  VariableDeclaration*,
  Assignment*>;

struct Root
{
  FuncList* funcList = nullptr;
  Scope* rootScope = nullptr;
};

struct FuncList
{
  Func* func = nullptr;
  FuncList* next = nullptr;
};

struct Func
{
  Type* returnType = nullptr;
  Id* name = nullptr;
  ArgList* argList = nullptr;
  Block* funcBody = nullptr;
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

struct Block
{
  std::vector<Statement*> statements;
  Scope* scope = nullptr;
};

struct VariableDeclaration
{
  Type* type = nullptr;
  Id* name = nullptr;
};

struct Assignment
{
  Expression* left = nullptr;
  Expression* right = nullptr;
};

struct ReturnStatement
{
  Expression* retval = nullptr;
};

struct Type
{
  std::string name;
};

struct CompareEqual
{
  Expression* left = nullptr;
  Expression* right = nullptr;
};

struct Scope
{
  Scope* parent = nullptr;
  std::unordered_map<std::string, VariableDeclaration*> variables;
};

#define FOR_EACH_AST_TYPE \
  X(Root) \
  X(FuncList) \
  X(Func) \
  X(Block) \
  X(Statement) \
  X(VariableDeclaration) \
  X(Assignment) \
  X(ReturnStatement) \
  X(Type) \
  X(ArgList) \
  X(Arg) \
  X(Expression) \
  X(Id) \
  X(CompareEqual)

# define X(Type) void dumpJson(const Type* node, std::string& str, int32_t tabIndex = 0);
FOR_EACH_AST_TYPE
# undef X
