#pragma once
#include <string>
#include <cstdint>
#include <variant>
#include <vector>
#include <unordered_map>
#include "Assert.hpp"

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

struct TypeRef
{
  Type* type = nullptr;
  int32_t pointerDepth = 0;

  bool operator==(const TypeRef&) const = default;
  bool operator!=(const TypeRef&) const = default;
};

struct Scope;

class Expression
{
public:
  enum Tag
  {
    None,
    Id,
    Int32,
    Op,
  };

  Expression() = default;
  Expression(std::string id) : _id(std::move(id)), _tag(Id) {}
  Expression(int32_t i32) : _i32(i32), _tag(Int32) {}
  Expression(struct Op* op) : _op(op), _tag(Op) {}

  Tag tag() const { return _tag; }

  bool isId() const { return _tag == Id; }
  bool isInt32() const { return _tag == Int32; }
  bool isOp() const { return _tag == Op; }

  const std::string& id() const { release_assert(isId()); return _id; }
  int32_t int32() const { release_assert(isInt32()); return _i32; }
  const struct Op* op() const { release_assert(isOp()); return _op; }
  struct Op* op() { release_assert(isOp()); return _op; }

public:
  TypeRef type;

private:
  Tag _tag = None;

  std::string _id;
  int32_t _i32 = 0;
  struct Op* _op = nullptr;
};

using Statement = std::variant<std::monostate,
  ReturnStatement*,
  VariableDeclaration*,
  Assignment*,
  Expression*,
  IfElseChain*>;

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
  std::vector<std::string> argsOrder;
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

struct IfElseChain
{
  std::vector<IfElseChainItem*> items;
};

struct IfElseChainItem
{
  Expression* condition = nullptr;
  Block* block = nullptr;
};

struct ScopeItem
{
  enum class Tag
  {
    None,
    Function,
    Variable,
  };

  ScopeItem() = default;
  ScopeItem(Func* func) : _tag(Tag::Function), _function(func) { release_assert(func); }
  ScopeItem(VariableDeclaration* var) : _tag(Tag::Variable), _variable(var) { release_assert(var); }

  Tag tag() const { return _tag; }
  bool isFunction() const { return _tag == Tag::Function; }
  bool isVariable() const { return _tag == Tag::Variable; }
  operator bool() const { return _tag != Tag::None; }

  Func* function() { release_assert(isFunction()); return _function; }
  const Func* function() const { release_assert(isFunction()); return _function; }
  VariableDeclaration* variable() { release_assert(isVariable()); return _variable; }
  const VariableDeclaration* variable() const{ release_assert(isVariable()); return _variable; }

private:
  Tag _tag = Tag::None;
  union
  {
    Func* _function;
    VariableDeclaration* _variable;
  };
};

struct Scope
{
  Scope* parent = nullptr;
  Scope* parent2 = nullptr; // function blocks need two parents - global scope + the function's arguments

  std::unordered_map<std::string, ScopeItem> variables;

  ScopeItem lookup(const std::string& name);
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
  X(Expression) \
  X(Op) \
  X(Class) \
  X(IfElseChain) \
  X(IfElseChainItem)

# define X(Type) void dumpJson(const Type* node, std::string& str, int32_t tabIndex = 0);
FOR_EACH_AST_TYPE
# undef X

