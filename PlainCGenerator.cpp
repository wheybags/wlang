#include "PlainCGenerator.hpp"
#include "StringUtil.hpp"
#include "Assert.hpp"
#include <unordered_map>


PlainCGenerator::PlainCGenerator()
{
  headers = "#include <stdint.h>\n";
}

std::string PlainCGenerator::generate(const Root *root)
{
  generate(root, 0);
  return headers + "\n" + classes + functionPrototypes + "\n" + functionBodies;
}

static const std::unordered_map<std::string, std::string> builtinTypeMapping
{
  {"i32", "int32_t"},
  {"bool", "bool"}
};

void PlainCGenerator::generate(const Root* node, int32_t tabIndex)
{
  generate(node->funcList, tabIndex);
}

void PlainCGenerator::generate(const FuncList* node, int32_t tabIndex)
{
  for (Class* c : node->classes)
    generate(c);

  for (Func* func : node->functions)
    generate(func, tabIndex);
}

void PlainCGenerator::generate(const Func* node, int32_t tabIndex)
{
  std::string prototype;
  {
    prototype = builtinTypeMapping.at(node->returnType->name) + " " + node->name + "(";

    ArgList *argNode = node->argList;
    while (argNode)
    {
      prototype += builtinTypeMapping.at(argNode->arg->type->name) + " " + argNode->arg->name + ", ";
      argNode = argNode->next;
    }

    if (Str::endsWith(prototype, ", "))
      prototype.resize(prototype.size() - 2);

    prototype += ")";
  }

  functionPrototypes += prototype + ";\n";

  functionBodies += Str::space(tabIndex) + prototype + "\n{\n";
  tabIndex++;
  {
    for (const Statement* statement : node->funcBody->statements)
    {
      functionBodies += Str::space(tabIndex);
      generate(statement, functionBodies);
      functionBodies += "\n";
    }
  }
  tabIndex--;
  functionBodies += "}\n\n";
}

void PlainCGenerator::generate(const Statement* node, std::string& str)
{
  if (std::holds_alternative<ReturnStatement*>(*node))
  {
    str += "return ";
    generate(std::get<ReturnStatement*>(*node)->retval, str);
  }
  else if (std::holds_alternative<VariableDeclaration*>(*node))
  {
    VariableDeclaration* variableDeclaration = std::get<VariableDeclaration*>(*node);
    str += builtinTypeMapping.at(variableDeclaration->type->name) + " " + variableDeclaration->name;
    if (variableDeclaration->initialiser)
    {
      str += " = ";
      generate(variableDeclaration->initialiser, str);
    }
  }
  else if (std::holds_alternative<Assignment*>(*node))
  {
    Assignment* assignment = std::get<Assignment*>(*node);
    generate(assignment->left, str);
    str += " = ";
    generate(assignment->right, str);
  }
  else if (std::holds_alternative<Expression*>(*node))
  {
    generate(std::get<Expression*>(*node), str);
  }
  else
  {
    message_and_abort("bad Statement");
  }

  str += ";";
}

void PlainCGenerator::generate(const Expression* node, std::string& str)
{
  if (std::holds_alternative<std::string>(*node))
  {
    str += std::get<std::string>(*node);
  }
  else if (std::holds_alternative<int32_t>(*node))
  {
    str += std::to_string(std::get<int32_t>(*node));
  }
  else if (std::holds_alternative<Op*>(*node))
  {
    const Op* opNode = std::get<Op*>(*node);

    switch (opNode->type)
    {
      case Op::Type::Add:
      {
        str += "(";
        generate(opNode->left, str);
        str += " + ";
        generate(opNode->right, str);
        str += ")";
        break;
      }
      case Op::Type::Subtract:
      {
        str += "(";
        generate(opNode->left, str);
        str += " - ";
        generate(opNode->right, str);
        str += ")";
        break;
      }
      case Op::Type::Multiply:
      {
        str += "(";
        generate(opNode->left, str);
        str += " * ";
        generate(opNode->right, str);
        str += ")";
        break;
      }
      case Op::Type::Divide:
      {
        str += "(";
        generate(opNode->left, str);
        str += " / ";
        generate(opNode->right, str);
        str += ")";
        break;
      }
      case Op::Type::CompareEqual:
      {
        str += "(";
        generate(opNode->left, str);
        str += " == ";
        generate(opNode->right, str);
        str += ")";
        break;
      }
      case Op::Type::CompareNotEqual:
      {
        str += "(";
        generate(opNode->left, str);
        str += " != ";
        generate(opNode->right, str);
        str += ")";
        break;
      }
      case Op::Type::LogicalAnd:
      {
        str += "(";
        generate(opNode->left, str);
        str += " && ";
        generate(opNode->right, str);
        str += ")";
        break;
      }
      case Op::Type::LogicalOr:
      {
        str += "(";
        generate(opNode->left, str);
        str += " || ";
        generate(opNode->right, str);
        str += ")";
        break;
      }
      case Op::Type::LogicalNot:
      {
        str += "(!";
        generate(opNode->left, str);
        str += ")";
        break;
      }
      case Op::Type::UnaryMinus:
      {
        str += "(-";
        generate(opNode->left, str);
        str += ")";
        break;
      }
      case Op::Type::Call:
      {
        str += "(";
        generate(opNode->left, str);
        str += ")(";
        for (int32_t i = 0; i < int32_t(opNode->callArgs.size()); i++)
        {
          generate(opNode->callArgs[i], str);
          if (i != int32_t(opNode->callArgs.size()) - 1)
            str += ", ";
        }
        str += ")";
        break;
      }
      case Op::Type::ENUM_END:
        message_and_abort("bad enum");
    }
  }
  else
  {
    message_and_abort("bad Expression");
  }
}

void PlainCGenerator::generate(const Class* node)
{
  this->classes += "struct " + node->type->name + "\n";
  this->classes += "{\n";

  this->functionPrototypes += "void " + node->type->name + "__init_empty(" + node->type->name + "* obj);\n";
  this->functionBodies += "void " + node->type->name + "__init_empty(" + node->type->name + "* obj)\n{\n";

  for (int32_t i = 0; i < int32_t(node->memberVariables.size()); i++)
  {
    this->classes += "  ";

    VariableDeclaration* variableDeclaration = node->memberVariables[i];
    this->classes += builtinTypeMapping.at(variableDeclaration->type->name) + " " + variableDeclaration->name;

    this->functionBodies += "  obj->" + variableDeclaration->name + " = ";
    generate(variableDeclaration->initialiser, this->functionBodies);
    this->functionBodies += ";\n";

    this->classes += ";\n";
  }

  this->functionBodies += "}\n\n";
  this->classes += "};\n\n";
}

