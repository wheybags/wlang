#include "Ast.hpp"
#include "Assert.hpp"
#include <vector>

using Value = std::variant<
# define X(Type) Type*, std::vector<Type*>,
  FOR_EACH_AST_TYPE
# undef X

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

# define X(Type) \
  else if (std::holds_alternative<Type*>(value)) \
    dumpJson(std::get<Type*>(value), str, tabIndex); \
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
  if (std::holds_alternative<std::string>(*node))
    dumpJson(std::get<std::string>(*node), str, tabIndex);
  else if (std::holds_alternative<int32_t>(*node))
    dumpJson(std::get<int32_t>(*node), str, tabIndex);
  else if (std::holds_alternative<Op*>(*node))
    dumpJson(std::get<Op*>(*node), str, tabIndex);
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
  dumpJson(
  {
    {"nodeType", "Func"},
    {"returnType", node->returnType},
    {"name", node->name},
    {"argList", node->argList},
    {"funcBody", node->funcBody}
  }, str, tabIndex);
}

void dumpJson(const ArgList* node, std::string& str, int32_t tabIndex)
{
  std::vector<Value> values;
  while (node)
  {
    values.push_back(node->arg);
    node = node->next;
  }
  dumpJson(values, str, tabIndex);
}

void dumpJson(const Arg* node, std::string& str, int32_t tabIndex)
{
  dumpJson({{"nodeType", "Arg"}, {"type", node->type}, {"name", node->name}}, str, tabIndex);
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
  std::vector<std::pair<std::string, Value>> values {{"nodeType", "VariableDeclaration"}, {"type", node->type}, {"name", node->name}};
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
  dumpJson({{"nodeType", "Class"}, {"type", node->type}, {"memberVariables", node->memberVariables}}, str, tabIndex);
}

