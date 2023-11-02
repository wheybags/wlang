#include "PlainCGenerator.hpp"
#include "StringUtil.hpp"
#include "Assert.hpp"
#include <unordered_map>


PlainCGenerator::PlainCGenerator()
{
  headers.appendLine("#include <stdint.h>");
}

std::string PlainCGenerator::generate(const Root *root)
{
  generate(root->funcList);
  return headers.str + "\n" + classes.str + functionPrototypes.str + "\n" + functionBodies.str;
}

static const std::unordered_map<std::string, std::string> builtinTypeMapping
{
  {"i32", "int32_t"},
  {"bool", "bool"}
};

void PlainCGenerator::generate(const FuncList* node)
{
  for (Class* c : node->classes)
    generate(c);

  for (Func* func : node->functions)
    generate(func);
}

void PlainCGenerator::generate(const Func* node)
{
  std::string prototype;
  {
    prototype = strType(node->returnType) + " " + node->name + "(";

    for (const std::string& name : node->argsOrder)
    {
      ScopeItem item = node->argsScope->lookup(name);
      release_assert(item.isVariable());
      prototype += generate(item.variable());
      prototype += ", ";
    }

    if (Str::endsWith(prototype, ", "))
      prototype.resize(prototype.size() - 2);

    prototype += ")";
  }

  functionPrototypes.appendLine(prototype + ";");

  functionBodies.appendLine(prototype);
  generate(node->funcBody, functionBodies);
  functionBodies.appendLine();
}

void PlainCGenerator::generate(const Block* block, OutputString& str)
{
  str.appendLine("{");
  {
    for (const Statement* statement : block->statements)
      generate(statement, str);
  }
  str.appendLine("}");
}

void PlainCGenerator::generate(const Statement* node, OutputString& str)
{
  if (std::holds_alternative<ReturnStatement*>(*node))
  {
    str.appendLine("return " + generate(std::get<ReturnStatement*>(*node)->retval) + ";");
  }
  else if (std::holds_alternative<VariableDeclaration*>(*node))
  {
    str.appendLine(generate(std::get<VariableDeclaration*>(*node)) + ";");
  }
  else if (std::holds_alternative<Assignment*>(*node))
  {
    Assignment* assignment = std::get<Assignment*>(*node);
    str.appendLine(generate(assignment->left) + " = " +  generate(assignment->right) + ";");
  }
  else if (std::holds_alternative<Expression*>(*node))
  {
    str.appendLine(generate(std::get<Expression*>(*node)) + ";");
  }
  else if (std::holds_alternative<IfElseChain*>(*node))
  {
    IfElseChain* ifElseChain = std::get<IfElseChain*>(*node);
    for (int32_t i = 0; i < int32_t(ifElseChain->items.size()); i++)
    {
      IfElseChainItem* item = ifElseChain->items[i];

      std::string line;
      if (i == 0)
        line += "if";
      else if (item->condition)
        line += "else if";
      else
        line += "else";

      if (item->condition)
      {
        line += " (";
        line += generate(item->condition);
        line += ")";
      }

      str.appendLine(line);
      generate(item->block, str);
    }
  }
  else
  {
    message_and_abort("bad Statement");
  }
}

std::string PlainCGenerator::generate(const VariableDeclaration* variableDeclaration)
{
  Class* typeClass = variableDeclaration->type.pointerDepth == 0 ? variableDeclaration->type.type->typeClass : nullptr;

  std::string str;
  if (typeClass)
  {
    release_assert(!variableDeclaration->initialiser && "not supported yet");
    str += strType(variableDeclaration->type) + " " + variableDeclaration->name + "; ";
    str += strType(variableDeclaration->type) + "__init_empty(&" + variableDeclaration->name + ")";
  }
  else
  {
    str += strType(variableDeclaration->type) + " " + variableDeclaration->name;
  }

  if (variableDeclaration->initialiser)
  {
    str += " = ";
    str += generate(variableDeclaration->initialiser);
  }

  return str;
}

std::string PlainCGenerator::generate(const Expression* node)
{
  std::string str;

  switch(node->tag())
  {
    case Expression::Id:
    {
      str += node->id();
      break;
    }

    case Expression::Int32:
    {
      str += std::to_string(node->int32());
      break;
    }

    case Expression::Bool:
    {
      str += node->boolean() ? "true" : "false";
      break;
    }

    case Expression::Op:
    {
      const Op* opNode = node->op();

      switch (opNode->type)
      {
        case Op::Type::Add:
        {
          str += "(";
          str += generate(opNode->left);
          str += " + ";
          str += generate(opNode->right);
          str += ")";
          break;
        }
        case Op::Type::Subtract:
        {
          str += "(";
          str += generate(opNode->left);
          str += " - ";
          str += generate(opNode->right);
          str += ")";
          break;
        }
        case Op::Type::Multiply:
        {
          str += "(";
          str += generate(opNode->left);
          str += " * ";
          str += generate(opNode->right);
          str += ")";
          break;
        }
        case Op::Type::Divide:
        {
          str += "(";
          str += generate(opNode->left);
          str += " / ";
          str += generate(opNode->right);
          str += ")";
          break;
        }
        case Op::Type::CompareEqual:
        {
          str += "(";
          str += generate(opNode->left);
          str += " == ";
          str += generate(opNode->right);
          str += ")";
          break;
        }
        case Op::Type::CompareNotEqual:
        {
          str += "(";
          str += generate(opNode->left);
          str += " != ";
          str += generate(opNode->right);
          str += ")";
          break;
        }
        case Op::Type::LogicalAnd:
        {
          str += "(";
          str += generate(opNode->left);
          str += " && ";
          str += generate(opNode->right);
          str += ")";
          break;
        }
        case Op::Type::LogicalOr:
        {
          str += "(";
          str += generate(opNode->left);
          str += " || ";
          str += generate(opNode->right);
          str += ")";
          break;
        }
        case Op::Type::MemberAccess:
        {
          str += "(";
          str += generate(opNode->left);

          if (opNode->left->type.pointerDepth == 0)
            str += ".";
          else
            str += "->";

          str += generate(opNode->right);
          str += ")";
          break;
        }
        case Op::Type::LogicalNot:
        {
          str += "(!";
          str += generate(opNode->left);
          str += ")";
          break;
        }
        case Op::Type::UnaryMinus:
        {
          str += "(-";
          str += generate(opNode->left);
          str += ")";
          break;
        }
        case Op::Type::Call:
        {
          str += "(";
          str += generate(opNode->left);
          str += ")(";
          for (int32_t i = 0; i < int32_t(opNode->callArgs.size()); i++)
          {
            str += generate(opNode->callArgs[i]);
            if (i != int32_t(opNode->callArgs.size()) - 1)
              str += ", ";
          }
          str += ")";
          break;
        }
        case Op::Type::ENUM_END:
          message_and_abort("bad enum");
      }

      break;
    }

    case Expression::None:
      release_assert(false);
  }

  return str;
}

void PlainCGenerator::generate(const Class* node)
{
  this->classes.appendLine("struct " + node->type->name);
  this->classes.appendLine("{");

  this->functionPrototypes.appendLine("void " + node->type->name + "__init_empty(" + node->type->name + "* obj);");
  this->functionBodies.appendLine("void " + node->type->name + "__init_empty(" + node->type->name + "* obj)");
  this->functionBodies.appendLine("{");

  for (const std::string& name : node->memberVariableOrder)
  {
    VariableDeclaration* variableDeclaration = node->memberVariables.at(name);
    this->classes.appendLine(strType(variableDeclaration->type) + " " + variableDeclaration->name + ";");
    this->functionBodies.appendLine("obj->" + variableDeclaration->name + " = " + generate(variableDeclaration->initialiser));
  }

  this->functionBodies.appendLine("}");
  this->functionBodies.appendLine();
  this->classes.appendLine("};");
  this->classes.appendLine();
}

std::string PlainCGenerator::strType(const TypeRef& type)
{
  std::string str;

  if (type.type->typeClass)
    str += type.type->name;
  else
    str += builtinTypeMapping.at(type.type->name);

  for (int i = 0; i < type.pointerDepth; i++)
    str += "*";

  return str;
}

