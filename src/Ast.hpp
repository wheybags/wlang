#pragma once
#include <string>
#include <cstdint>
#include <variant>
#include <vector>
#include <unordered_map>
#include "Common/Assert.hpp"
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


struct ScopeId
{
  #define FOR_EACH_TAGGED_UNION_TYPE(XX) \
    XX(function, Function, Func*) \
    XX(variableDeclaration, VariableDeclaration, VariableDeclaration*) \
    XX(type, Type, Type*)
  #define CLASS_NAME Resolved
  #include "CreateTaggedUnion.hpp"

  ScopeId() {}
  ScopeId(std::string str): str(std::move(str)) {}
  ScopeId(std::string str, Resolved resolved) : str(std::move(str)), resolved(std::move(resolved)) {}
  ScopeId& operator=(std::string str) { this->str = std::move(str); return *this; }

  void resolveFunction(Scope& scope);
  void resolveVariableDeclaration(Scope& scope);
  void resolveType(Scope& scope);

public:
  std::string str;
  Resolved resolved;
};


struct TypeRef
{
  ScopeId id;
  int32_t pointerDepth = 0;

  bool operator==(const TypeRef& other) const;
  bool operator!=(const TypeRef& other) const;
};

struct Type
{
  std::string name;
  Class* typeClass = nullptr; // user defined types will have a class, builtins have only name
  bool builtin = false;
  bool builtinNumeric = false;

  TypeRef reference() { return { .id = ScopeId(name, this) }; }
};

struct IntegerConstant
{
  int64_t val = 0;
  int32_t size = 0;

  Type* getType() const;
};

struct StringConstant
{
  std::string val;
};

class Null {};

class Expression
{
public:
  #define FOR_EACH_TAGGED_UNION_TYPE(XX) \
    XX(id, Id, ScopeId) \
    XX(integerConstant, IntegerConstant, IntegerConstant) \
    XX(stringConstant, StringConstant, StringConstant) \
    XX(boolean, Bool, bool ) \
    XX(op, Op, Op*) \
    XX(null, Null, Null)
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
  std::string mangledName;
  std::vector<VariableDeclaration*> args;
  bool external = false; // this is an extern function declaration, will be linked in from a non-wlang shared object
  Class* memberClass = nullptr; // if this function is a member of a class, this will be set

  // not set if external is true
  Scope* argsScope = nullptr;
  Block* funcBody = nullptr;
};

struct Class
{
  Type* type = nullptr;
  std::vector<VariableDeclaration*> memberVariables;
  Scope* memberScope = nullptr;
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
    Subscript,
    Add,
    Subtract,
    Multiply,
    Divide,
    MemberAccess,
    AddressOf,
    ENUM_END
  };

  struct Binary
  {
    Expression* left = nullptr;
    Expression* right = nullptr;
  };

  struct Unary
  {
    Expression* expression = nullptr;
  };

  struct Call
  {
    Expression* callable = nullptr;
    std::vector<Expression*> callArgs;
  };

  struct Subscript
  {
    Expression* item = nullptr;
    Expression* index = nullptr;
  };

  struct MemberAccess
  {
    Expression* expression = nullptr;
    ScopeId member;
  };

  #define FOR_EACH_TAGGED_UNION_TYPE(XX) \
    XX(binary, Binary, Binary) \
    XX(unary, Unary, Unary) \
    XX(call, Call, Call) \
    XX(subscript, Subscript, Subscript) \
    XX(memberAccess, MemberAccess, MemberAccess)
  #define CLASS_NAME Args
  #include "CreateTaggedUnion.hpp"

  Type type = Type::ENUM_END;
  Args args;
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

private:
  template<typename T>
  T* lookup(ScopeId& name);
  friend struct ScopeId;
};
