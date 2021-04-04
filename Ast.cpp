#include "Ast.hpp"
#include "Assert.hpp"
#include <vector>

using Value = std::variant<std::string,
  int32_t,
  Root*,
  FuncList*,
  Func*,
  FuncBody*,
  ReturnType*,
  Type*,
  ArgList*,
  Arg*,
  Expression*,
  Id*,
  CompareEqual*>;

void dumpJson(const Value& value, std::string& str, int32_t tabIndex)
{
  if (std::holds_alternative<std::string>(value))
    str += "\"" + std::get<std::string>(value) + "\"";
  else if (std::holds_alternative<int32_t>(value))
    str += std::to_string(std::get<int32_t>(value));
  else if (std::holds_alternative<Root *>(value))
    dumpJson(std::get<Root *>(value), str, tabIndex);
  else if (std::holds_alternative<FuncList *>(value))
    dumpJson(std::get<FuncList *>(value), str, tabIndex);
  else if (std::holds_alternative<Func *>(value))
    dumpJson(std::get<Func *>(value), str, tabIndex);
  else if (std::holds_alternative<FuncBody *>(value))
    dumpJson(std::get<FuncBody *>(value), str, tabIndex);
  else if (std::holds_alternative<ReturnType *>(value))
    dumpJson(std::get<ReturnType *>(value), str, tabIndex);
  else if (std::holds_alternative<Type *>(value))
    dumpJson(std::get<Type *>(value), str, tabIndex);
  else if (std::holds_alternative<ArgList *>(value))
    dumpJson(std::get<ArgList *>(value), str, tabIndex);
  else if (std::holds_alternative<Arg *>(value))
    dumpJson(std::get<Arg *>(value), str, tabIndex);
  else if (std::holds_alternative<Expression *>(value))
    dumpJson(std::get<Expression *>(value), str, tabIndex);
  else if (std::holds_alternative<Id *>(value))
    dumpJson(std::get<Id *>(value), str, tabIndex);
  else if (std::holds_alternative<CompareEqual *>(value))
    dumpJson(std::get<CompareEqual *>(value), str, tabIndex);
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

void dumpJson(const std::vector<Value>& values, std::string& str, int32_t tabIndex)
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
  if (std::holds_alternative<Id*>(*node))
   dumpJson(std::get<Id*>(*node), str, tabIndex);
  else if (std::holds_alternative<int32_t>(*node))
    dumpJson(std::get<int32_t>(*node), str, tabIndex);
  else if (std::holds_alternative<CompareEqual*>(*node))
    dumpJson(std::get<CompareEqual*>(*node), str, tabIndex);
}

void dumpJson(const Root* node, std::string& str, int32_t tabIndex)
{
  dumpJson({{"nodeType", "Root"}, {"funcList", node->funcList}}, str, tabIndex);
}

void dumpJson(const FuncList* node, std::string& str, int32_t tabIndex)
{
  std::vector<Value> values;
  while (node)
  {
    values.push_back(node->func);
    node = node->next;
  }
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

void dumpJson(const Id* node, std::string& str, int32_t tabIndex)
{
  dumpJson({{"nodeType", "Id"}, {"name", node->name}}, str, tabIndex);
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

void dumpJson(const FuncBody* node, std::string& str, int32_t tabIndex)
{
  dumpJson({{"nodeType", "FuncBody"}, {"retval", node->retval}}, str, tabIndex);
}

void dumpJson(const ReturnType* node, std::string& str, int32_t tabIndex)
{
  dumpJson({{"nodeType", "ReturnType"}, {"type", node->type}}, str, tabIndex);
}

void dumpJson(const Type* node, std::string& str, int32_t tabIndex)
{
  dumpJson({{"nodeType", "Type"}, {"id", node->id}}, str, tabIndex);
}

void dumpJson(const CompareEqual* node, std::string& str, int32_t tabIndex)
{
  dumpJson({{"nodeType", "CompareEqual"}, {"left", node->left}, {"right", node->right}}, str, tabIndex);
}
