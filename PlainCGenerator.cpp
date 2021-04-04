#include "PlainCGenerator.hpp"
#include "StringUtil.hpp"
#include <unordered_map>


PlainCGenerator::PlainCGenerator()
{
  headers = "#include <stdint.h>\n";
}

std::string PlainCGenerator::generate(const Root *root)
{
  generate(root, 0);
  return headers + "\n" + functionPrototypes + "\n" + functionBodies;
}

static const std::unordered_map<std::string, std::string> builtinTypeMapping
{
  {"i32", "int32_t"}
};

void PlainCGenerator::generate(const Root* node, int32_t tabIndex)
{
  generate(node->funcList, tabIndex);
}

void PlainCGenerator::generate(const FuncList* node, int32_t tabIndex)
{
  while (node)
  {
    generate(node->func, tabIndex);
    node = node->next;
  }
}

void PlainCGenerator::generate(const Func* node, int32_t tabIndex)
{
  std::string prototype;
  {
    prototype = builtinTypeMapping.at(node->returnType->type->id->name) + " " + node->name->name + "(";

    ArgList *argNode = node->argList;
    while (argNode)
    {
      prototype += builtinTypeMapping.at(argNode->arg->type->id->name) + " " + argNode->arg->name->name + ",";
      argNode = argNode->next;
    }

    if (Str::endsWith(prototype, ","))
      prototype.resize(prototype.size() - 1);

    prototype += ")";
  }

  functionPrototypes += prototype + ";\n";

  functionBodies += Str::space(tabIndex) + prototype + "\n{\n";
  tabIndex++;
  {
    functionBodies += Str::space(tabIndex) + "return ";
    generate(node->funcBody->retval, functionBodies, 0);
    functionBodies += ";\n";
  }
  tabIndex--;
  functionBodies += "}\n\n";
}

void PlainCGenerator::generate(const Expression* node, std::string& str, int32_t tabIndex)
{
  str += Str::space(tabIndex);

  if (std::holds_alternative<Id*>(*node))
    str += std::get<Id*>(*node)->name;
  else if (std::holds_alternative<int32_t>(*node))
    str += std::to_string(std::get<int32_t>(*node));
  else if (std::holds_alternative<CompareEqual*>(*node))
  {
    const CompareEqual* compareNode = std::get<CompareEqual*>(*node);
    generate(compareNode->left, str, 0);
    str += " == ";
    generate(compareNode->right, str, 0);
  }
}

