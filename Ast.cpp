#include "Ast.hpp"
#include "Assert.hpp"
#include <vector>

void dumpJson(const TypeRef* typeRef, std::string& str, int32_t tabIndex = 0);

using Value = std::variant<
# define X(Type) const Type*, std::vector<Type*>,
  FOR_EACH_AST_TYPE
# undef X

  const TypeRef*,
  std::string,
  int32_t>;

template <typename T>
void dumpJson(const std::vector<T>& values, std::string& str, int32_t tabIndex);


void dumpJson(const Value& value, std::string& str, int32_t tabIndex)
{
  if (std::holds_alternative<std::string>(value))
    str += "\"" + std::get<std::string>(value) + "\"";
  else if (std::holds_alternative<int32_t>(value))
    str += std::to_string(std::get<int32_t>(value));
  else if (std::holds_alternative<const TypeRef*>(value))
   dumpJson(std::get<const TypeRef*>(value), str, tabIndex);

# define X(Type) \
  else if (std::holds_alternative<const Type*>(value)) \
    dumpJson(std::get<const Type*>(value), str, tabIndex); \
  else if (std::holds_alternative<std::vector<Type*>>(value)) \
    dumpJson(std::get<std::vector<Type*>>(value), str, tabIndex);

  FOR_EACH_AST_TYPE
#undef X
  else
    message_and_abort("unknown node");
}

void dumpJson(const std::vector<std::pair<std::string, Value>>& values, std::string& str, int32_t tabIndex)
{
  str += "{\n";
  tabIndex++;

  for (const auto& value : values)
  {
    str += std::string(tabIndex * 2, ' ') + "\"" + value.first + "\": ";
    dumpJson(value.second, str, tabIndex);
    str += ",\n";
  }

  if (str[str.size() - 2] == ',')
  {
    str.resize(str.size() - 1);
    str[str.size() - 1] = '\n';
  }

  tabIndex--;
  str += std::string(tabIndex * 2, ' ') + "}";
}

template <typename T>
void dumpJson(const std::vector<T>& values, std::string& str, int32_t tabIndex)
{
  str += "[\n";
  tabIndex++;

  for (const auto& value : values)
  {
    str += std::string(tabIndex * 2, ' ');
    dumpJson(value, str, tabIndex);
    str += ",\n";
  }

  if (str[str.size() - 2] == ',')
  {
    str.resize(str.size() - 1);
    str[str.size() - 1] = '\n';
  }

  tabIndex--;
  str += std::string(tabIndex * 2, ' ') + "]";
}

void dumpJson(const Expression* node, std::string& str, int32_t tabIndex)
{
  if (node->isId())
    dumpJson(node->id(), str, tabIndex);
  else if (node->isInt32())
    dumpJson(node->int32(), str, tabIndex);
  else if (node->isOp())
    dumpJson(node->op(), str, tabIndex);
  else
    message_and_abort("empty expression");
}

void dumpJson(const Statement* node, std::string& str, int32_t tabIndex)
{
  if (std::holds_alternative<ReturnStatement*>(*node))
    dumpJson(std::get<ReturnStatement*>(*node), str, tabIndex);
  else if (std::holds_alternative<VariableDeclaration*>(*node))
    dumpJson(std::get<VariableDeclaration*>(*node), str, tabIndex);
  else if (std::holds_alternative<Assignment*>(*node))
    dumpJson(std::get<Assignment*>(*node), str, tabIndex);
  else if (std::holds_alternative<Expression*>(*node))
    dumpJson(std::get<Expression*>(*node), str, tabIndex);
  else if (std::holds_alternative<IfElseChain*>(*node))
    dumpJson(std::get<IfElseChain*>(*node), str, tabIndex);
  else
    message_and_abort("empty expression");
}

void dumpJson(const Root* node, std::string& str, int32_t tabIndex)
{
  dumpJson({{"nodeType", "Root"}, {"funcList", node->funcList}, {"classes", node->funcList->classes}}, str, tabIndex);
}

void dumpJson(const FuncList* node, std::string& str, int32_t tabIndex)
{
  std::vector<Value> values;
  values.reserve(node->functions.size());
  for (Func* func : node->functions)
    values.emplace_back(func);
  dumpJson(values, str, tabIndex);
}

void dumpJson(const Func* node, std::string& str, int32_t tabIndex)
{
  std::vector<VariableDeclaration*> values;
  values.reserve(node->argsOrder.size());
  for (const std::string& name : node->argsOrder)
    values.emplace_back(node->argsScope->lookup(name).variable());

  dumpJson(
  {
    {"nodeType", "Func"},
    {"returnType", &node->returnType},
    {"name", node->name},
    {"argList", values},
    {"funcBody", node->funcBody}
  }, str, tabIndex);
}

void dumpJson(const Block* node, std::string& str, int32_t tabIndex)
{
  std::vector<Value> values;
  values.reserve(node->statements.size());
  for (const auto& statement : node->statements)
    values.emplace_back(statement);
  dumpJson(values, str, tabIndex);
}

void dumpJson(const VariableDeclaration* node, std::string& str, int32_t tabIndex)
{
  std::vector<std::pair<std::string, Value>> values {{"nodeType", "VariableDeclaration"}, {"type", &node->type}, {"name", node->name}};
  if (node->initialiser)
    values.emplace_back("initialiser", node->initialiser);
  dumpJson(values, str, tabIndex);
}

void dumpJson(const Assignment* node, std::string& str, int32_t tabIndex)
{
  dumpJson({{"nodeType", "Assignment"}, {"left", node->left}, {"right", node->right}}, str, tabIndex);
}

void dumpJson(const ReturnStatement* node, std::string& str, int32_t tabIndex)
{
  dumpJson({{"nodeType", "ReturnStatement"}, {"retval", node->retval}}, str, tabIndex);
}

void dumpJson(const Type* node, std::string& str, int32_t tabIndex)
{
  dumpJson({{"nodeType", "Type"}, {"name", node->name}}, str, tabIndex);
}


void dumpJson(const Op* node, std::string& str, int32_t tabIndex)
{
  std::string op;
  switch (node->type)
  {
    case Op::Type::Add:
    {
      dumpJson({{"nodeType", "OpAdd"}, {"left", node->left}, {"right", node->right}}, str, tabIndex);
      break;
    }
    case Op::Type::Subtract:
    {
      dumpJson({{"nodeType", "OpSubtract"}, {"left", node->left}, {"right", node->right}}, str, tabIndex);
      break;
    }
    case Op::Type::Multiply:
    {
      dumpJson({{"nodeType", "OpMultiply"}, {"left", node->left}, {"right", node->right}}, str, tabIndex);
      break;
    }
    case Op::Type::Divide:
    {
      dumpJson({{"nodeType", "OpDivide"}, {"left", node->left}, {"right", node->right}}, str, tabIndex);
      break;
    }
    case Op::Type::CompareEqual:
    {
      dumpJson({{"nodeType", "OpCompareEqual"}, {"left", node->left}, {"right", node->right}}, str, tabIndex);
      break;
    }
    case Op::Type::CompareNotEqual:
    {
      dumpJson({{"nodeType", "OpCompareNotEqual"}, {"left", node->left}, {"right", node->right}}, str, tabIndex);
      break;
    }
    case Op::Type::LogicalAnd:
    {
      dumpJson({{"nodeType", "OpLogicalAnd"}, {"left", node->left}, {"right", node->right}}, str, tabIndex);
      break;
    }
    case Op::Type::LogicalOr:
    {
      dumpJson({{"nodeType", "OpLogicalOr"}, {"left", node->left}, {"right", node->right}}, str, tabIndex);
      break;
    }
     case Op::Type::MemberAccess:
    {
      dumpJson({{"nodeType", "OpMemberAccess"}, {"left", node->left}, {"right", node->right}}, str, tabIndex);
      break;
    }
    case Op::Type::LogicalNot:
    {
      dumpJson({{"nodeType", "OpLogicalNot"}, {"expression", node->left}}, str, tabIndex);
      break;
    }
    case Op::Type::UnaryMinus:
    {
      dumpJson({{"nodeType", "OpUnaryMinus"}, {"expression", node->left}}, str, tabIndex);
      break;
    }
    case Op::Type::Call:
    {
      dumpJson({{"nodeType", "OpCall"}, {"callable", node->left}, {"arguments", node->callArgs}}, str, tabIndex);
      break;
    }
    case Op::Type::ENUM_END:
      release_assert(false);
  }
}

void dumpJson(const Class* node, std::string& str, int32_t tabIndex)
{
  std::vector<VariableDeclaration*> memberVariables;
  memberVariables.reserve(node->memberVariables.size());
  for (const auto& pair : node->memberVariables)
    memberVariables.emplace_back(pair.second);

  dumpJson({{"nodeType", "Class"}, {"type", node->type}, {"memberVariables", memberVariables}}, str, tabIndex);
}

void dumpJson(const TypeRef* typeRef, std::string& str, int32_t tabIndex)
{
  dumpJson({{"nodeType", "TypeRef"}, {"type", typeRef->type}, {"pointerDepth", typeRef->pointerDepth}}, str, tabIndex);
}

void dumpJson(const IfElseChain* ifElseChain, std::string& str, int32_t tabIndex)
{
  dumpJson({{"nodeType", "IfElseChain"}, {"items", ifElseChain->items}}, str, tabIndex);
}

void dumpJson(const IfElseChainItem* ifElseChainItem, std::string& str, int32_t tabIndex)
{
  std::vector<std::pair<std::string, Value>> values;
  values.emplace_back("nodeType", "IfElseChainItem");
  if (ifElseChainItem->condition)
    values.emplace_back("condition", ifElseChainItem->condition);
  values.emplace_back("block", ifElseChainItem->block);

  dumpJson(values, str, tabIndex);
}

ScopeItem Scope::lookup(const std::string& name)
{
  auto it = this->variables.find(name);
  if (it != this->variables.end())
    return it->second;

  if (this->parent2)
  {
    ScopeItem item = this->parent2->lookup(name);
    if (item)
      return item;
  }

  if (this->parent)
  {
    ScopeItem item = this->parent->lookup(name);
    if (item)
      return item;
  }

  return {};
}