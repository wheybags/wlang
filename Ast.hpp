#pragma once
#include <string>
#include <cstdint>
#include <variant>
#include <vector>
#include <unordered_map>
#include "Assert.hpp"
#include "Tokeniser.hpp"
#include "HashMap.hpp"

struct Root;
struct FuncList;
struct Func;
struct Class;
struct Block;
struct VariableDeclaration;
struct Assignment;
struct ReturnStatement;
struct Type;
struct Op;
struct IfElseChain;
struct IfElseChainItem;
struct Scope;
class AstChunk;

struct TypeRef
{
  std::string typeName;
  Type* typeResolved = nullptr;
  int32_t pointerDepth = 0;

  bool operator==(TypeRef& other);
  bool operator!=(TypeRef& other);
};

struct Type
{
  std::string name;
  bool builtin = false;
  Class* typeClass = nullptr; // user defined types will have a class, builtins have only name

  bool defined() const { return builtin || typeClass; }
  TypeRef reference() { return {.typeName = name, .typeResolved = this}; }
};

class Expression
{
public:
  #define FOR_EACH_TAGGED_UNION_TYPE(XX) \
    XX(id, Id, std::string) \
    XX(i32, Int32, int32_t) \
    XX(boolean, Bool, bool ) \
    XX(op, Op, Op*)
  #define CLASS_NAME Val
  #include "CreateTaggedUnion.hpp"

  Val val;
  TypeRef type;
  SourceRange source;
};


#define FOR_EACH_TAGGED_UNION_TYPE(XX) \
  XX(returnStatment, Return, ReturnStatement*) \
  XX(variable, Variable, VariableDeclaration*) \
  XX(assignment, Assignment, Assignment*) \
  XX(expression, Expression, Expression*) \
  XX(ifElseChain, IfElseChain, IfElseChain*)
#define CLASS_NAME Statement
#include "CreateTaggedUnion.hpp"


struct Root
{
  FuncList* funcList = nullptr;
};

struct FuncList
{
  Scope* scope = nullptr;
  std::vector<Func*> functions;
  std::vector<Class*> classes;
};

struct Func
{
  TypeRef returnType;
  std::string name;
  std::vector<VariableDeclaration*> args;
  Scope* argsScope = nullptr;
  Block* funcBody = nullptr;
};

struct Class
{
  Type* type = nullptr;
  std::vector<std::string> memberVariableOrder;
  std::unordered_map<std::string, VariableDeclaration*> memberVariables;
};

struct Block
{
  std::vector<Statement*> statements;
  Scope* scope = nullptr;
};

struct VariableDeclaration
{
  TypeRef type;
  std::string name;
  SourceRange source;
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

struct IfElseChain
{
  std::vector<IfElseChainItem*> items;
};

struct IfElseChainItem
{
  Expression* condition = nullptr;
  Block* block = nullptr;
};

struct Scope
{
  Scope* parent = nullptr;
  Scope* parent2 = nullptr; // function blocks need two parents - global scope + the function's arguments

  template<typename T>
  struct Item
  {
    T item;
    AstChunk* chunk;
  };

  HashMap<Item<Func*>> functions;
  HashMap<Item<VariableDeclaration*>> variables;
  HashMap<Item<Type*>> types;

  Func* lookupFunc(std::string_view name) { return lookup<Func>(name); }
  VariableDeclaration* lookupVar(std::string_view name) { return lookup<VariableDeclaration>(name); }
  Type* lookupType(std::string_view name) { return lookup<Type>(name); }

private:
  template<typename T>
  T* lookup(std::string_view name);
};
