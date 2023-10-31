#pragma once
#include <string>
#include <cstdint>
#include <variant>
#include <vector>
#include <unordered_map>

struct Root;
struct FuncList;
struct Func;
struct Class;
struct Block;
struct VariableDeclaration;
struct Assignment;
struct ReturnStatement;
struct Type;
struct ArgList;
struct Arg;
struct Op;

struct Scope;

using Expression = std::variant<std::monostate,
  std::string,
  int32_t,
  Op*
>;

using Statement = std::variant<std::monostate,
  ReturnStatement*,
  VariableDeclaration*,
  Assignment*,
  Expression*>;

struct Root
{
  FuncList* funcList = nullptr;
  Scope* rootScope = nullptr;
};

struct FuncList
{
  std::vector<Func*> functions;
  std::vector<Class*> classes;
};

struct Func
{
  Type* returnType = nullptr;
  std::string name;
  ArgList* argList = nullptr;
  Block* funcBody = nullptr;
};

struct Class
{
  Type* type = nullptr;
  std::vector<VariableDeclaration*> memberVariables;
};

struct ArgList
{
  Arg* arg = nullptr;
  ArgList* next = nullptr;
};

struct Arg
{
  Type* type = nullptr;
  std::string name;
};

struct Block
{
  std::vector<Statement*> statements;
  Scope* scope = nullptr;
};

struct VariableDeclaration
{
  Type* type = nullptr;
  std::string name;
  Expression* initialiser = nullptr; // maybe null
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

  // user defined types will have a class, builtins have only name
  Class* typeClass = nullptr;
};

struct Op
{
  enum class Type
  {
    CompareEqual,
    CompareNotEqual,
    LogicalAnd,
    LogicalOr,
    LogicalNot,
    UnaryMinus,
    Call,
    Add,
    Subtract,
    Multiply,
    Divide,
    MemberAccess,
    ENUM_END
  };

  Expression* left = nullptr;
  Expression* right = nullptr;

  std::vector<Expression*> callArgs;

  Type type = Type::ENUM_END;
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
  X(Op) \
  X(Class)

# define X(Type) void dumpJson(const Type* node, std::string& str, int32_t tabIndex = 0);
FOR_EACH_AST_TYPE
# undef X
