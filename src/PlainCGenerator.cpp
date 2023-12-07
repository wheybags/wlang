#include "PlainCGenerator.hpp"
#include "Common/StringUtil.hpp"
#include "Common/Assert.hpp"
#include <unordered_map>
#include <functional>


PlainCGenerator::PlainCGenerator() {}

std::string PlainCGenerator::output()
{
  OutputString declarations;

  std::vector<const Type*> sortedTypes;
  std::unordered_set<const Type*> allTypesUsedByReference = this->usedTypesByRef;

  std::unordered_set<const Type*> seen;
  std::unordered_set<const Type*> seenThisIteration;

  std::function<void(const Type*)> evalType = [&](const Type* type) -> void
  {
    release_assert(!seenThisIteration.contains(type)); // circular reference!

    bool wasSeen = seen.contains(type);
    seen.insert(type);
    seenThisIteration.insert(type);

    if (type->typeClass)
    {
      for (const VariableDeclaration* member: type->typeClass->memberVariables)
      {
        if (member->type.pointerDepth == 0)
        {
          evalType(member->type.id.resolved.type());
        }
        else
        {
          allTypesUsedByReference.emplace(member->type.id.resolved.type());
        }
      }
    }

    if (!wasSeen)
      sortedTypes.emplace_back(type);

    seenThisIteration.erase(type);
  };

  for (const Type* type : this->usedTypesByValue)
    evalType(type);

  for (const Type* type : allTypesUsedByReference)
    declarations.appendLine("struct " + type->name + ";");

  for (const Type* type : sortedTypes)
  {
    if (type->typeClass)
      this->generateClassDeclaration(type->typeClass, declarations);
  }

  for (const Func* function : this->usedFunctions)
    declarations.appendLine(this->getPrototype(function) + ";");

  return declarations.str + functionBodies.str;
}

void PlainCGenerator::generate(const Root *root)
{
  generate(root->funcList);
}

static const std::unordered_map<std::string, std::string> builtinTypeMapping
{
  {"i32", "int"},
  {"bool", "char"}
};

void PlainCGenerator::generate(const FuncList* node)
{
  for (Func* func : node->functions)
    generate(func);
}

std::string PlainCGenerator::getPrototype(const Func* node)
{
  std::string prototype= strType(node->returnType) + " " + node->name + "(";

  for (VariableDeclaration* var : node->args)
  {
    release_assert(!var->initialiser);
    prototype += strType(var->type) + " " + var->name;
    prototype += ", ";
  }

  if (Str::endsWith(prototype, ", "))
    prototype.resize(prototype.size() - 2);

  prototype += ")";

  return prototype;
}

void PlainCGenerator::generate(const Func* node)
{
  std::string prototype = getPrototype(node);

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
  switch (node->tag())
  {
    case Statement::Tag::Return:
    {
      str.appendLine("return " + generate(node->returnStatment()->retval) + ";");
      break;
    }
    case Statement::Tag::Variable:
    {
      str.appendLine(generate(node->variable()) + ";");
      break;
    }
    case Statement::Tag::Assignment:
    {
      Assignment* assignment = node->assignment();
      str.appendLine(generate(assignment->left) + " = " +  generate(assignment->right) + ";");
      break;
    }
    case Statement::Tag::Expression:
    {
      str.appendLine(generate(node->expression()) + ";");
      break;
    }
    case Statement::Tag::IfElseChain:
    {
      IfElseChain* ifElseChain = node->ifElseChain();
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
      break;
    }
    case Statement::Tag::None:
    {
      message_and_abort("bad Statement");
    }
  }
}

std::string PlainCGenerator::generate(const VariableDeclaration* variableDeclaration, bool suppressInitialiser)
{
  this->referenceType(variableDeclaration->type);

  Class* typeClass = variableDeclaration->type.id.resolved.type()->typeClass;

  std::string str;
  if (typeClass)
  {
    release_assert(!variableDeclaration->initialiser && "not supported yet");

    // TODO: reimplement this at the AST level?
    str += "struct " + strType(variableDeclaration->type) + " " + variableDeclaration->name;// + "; ";
//    str += strType(variableDeclaration->type) + "__init_empty(&" + variableDeclaration->name + ")";
  }
  else
  {
    str += strType(variableDeclaration->type) + " " + variableDeclaration->name;
  }

  if (!suppressInitialiser && variableDeclaration->initialiser)
  {
    str += " = ";
    str += generate(variableDeclaration->initialiser);
  }

  return str;
}

std::string PlainCGenerator::generate(const Expression* node)
{
  std::string str;

  switch(node->val.tag())
  {
    case Expression::Val::Tag::Id:
    {
      const ScopeId& scopeId = node->val.id();
      if (scopeId.resolved.isFunction())
        this->referenceFunction(scopeId.resolved.function());

      str += scopeId.str;
      break;
    }

    case Expression::Val::Tag::Int32:
    {
      str += std::to_string(node->val.i32());
      break;
    }

    case Expression::Val::Tag::Bool:
    {
      str += node->val.boolean() ? "1" : "0";
      break;
    }

    case Expression::Val::Tag::Op:
    {
      const Op* opNode = node->val.op();

      switch (opNode->type)
      {
        case Op::Type::Add:
        {
          const Op::Binary& binary = opNode->args.binary();
          str += "(";
          str += generate(binary.left);
          str += " + ";
          str += generate(binary.right);
          str += ")";
          break;
        }
        case Op::Type::Subtract:
        {
         const Op::Binary& binary = opNode->args.binary();
          str += "(";
          str += generate(binary.left);
          str += " - ";
          str += generate(binary.right);
          str += ")";
          break;
        }
        case Op::Type::Multiply:
        {
          const Op::Binary& binary = opNode->args.binary();
          str += "(";
          str += generate(binary.left);
          str += " * ";
          str += generate(binary.right);
          str += ")";
          break;
        }
        case Op::Type::Divide:
        {
         const Op::Binary& binary = opNode->args.binary();
          str += "(";
          str += generate(binary.left);
          str += " / ";
          str += generate(binary.right);
          str += ")";
          break;
        }
        case Op::Type::CompareEqual:
        {
          const Op::Binary& binary = opNode->args.binary();
          str += "(";
          str += generate(binary.left);
          str += " == ";
          str += generate(binary.right);
          str += ")";
          break;
        }
        case Op::Type::CompareNotEqual:
        {
          const Op::Binary& binary = opNode->args.binary();
          str += "(";
          str += generate(binary.left);
          str += " != ";
          str += generate(binary.right);
          str += ")";
          break;
        }
        case Op::Type::LogicalAnd:
        {
          const Op::Binary& binary = opNode->args.binary();
          str += "(";
          str += generate(binary.left);
          str += " && ";
          str += generate(binary.right);
          str += ")";
          break;
        }
        case Op::Type::LogicalOr:
        {
          const Op::Binary& binary = opNode->args.binary();
          str += "(";
          str += generate(binary.left);
          str += " || ";
          str += generate(binary.right);
          str += ")";
          break;
        }
        case Op::Type::MemberAccess:
        {
          const Op::MemberAccess& memberAccess = opNode->args.memberAccess();
          this->referenceType(memberAccess.expression->type);
          str += "(";
          str += generate(memberAccess.expression);

          if (memberAccess.expression->type.pointerDepth == 0)
            str += ".";
          else
            str += "->";

          str += memberAccess.member.str;
          str += ")";
          break;
        }
        case Op::Type::LogicalNot:
        {
          str += "(!";
          str += generate(opNode->args.unary().expression);
          str += ")";
          break;
        }
        case Op::Type::UnaryMinus:
        {
          str += "(-";
          str += generate(opNode->args.unary().expression);
          str += ")";
          break;
        }
        case Op::Type::Call:
        {
          const Op::Call& call = opNode->args.call();
          str += "(";
          str += generate(call.callable);
          str += ")(";
          for (int32_t i = 0; i < int32_t(call.callArgs.size()); i++)
          {
            str += generate(call.callArgs[i]);
            if (i != int32_t(call.callArgs.size()) - 1)
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

    case Expression::Val::Tag::None:
      release_assert(false);
  }

  return str;
}

void PlainCGenerator::generateClassDeclaration(const Class* node, OutputString& str)
{
  str.appendLine("struct " + node->type->name);
  str.appendLine("{");

  for (VariableDeclaration* variableDeclaration : node->memberVariables)
    str.appendLine(generate(variableDeclaration, true) + ";");

  str.appendLine("};");
  str.appendLine();
}

//void PlainCGenerator::generate(const Class* node)
//{
//  this->classes.appendLine("struct " + node->type->name);
//  this->classes.appendLine("{");
//
//  this->functionPrototypes.appendLine("void " + node->type->name + "__init_empty(" + node->type->name + "* obj);");
//  this->functionBodies.appendLine("void " + node->type->name + "__init_empty(" + node->type->name + "* obj)");
//  this->functionBodies.appendLine("{");
//
//  for (const std::string& name : node->memberVariableOrder)
//  {
//    VariableDeclaration* variableDeclaration = node->memberVariables.at(name);
//    this->classes.appendLine(strType(variableDeclaration->type) + " " + variableDeclaration->name + ";");
//    this->functionBodies.appendLine("obj->" + variableDeclaration->name + " = " + generate(variableDeclaration->initialiser) + ";");
//  }
//
//  this->functionBodies.appendLine("}");
//  this->functionBodies.appendLine();
//  this->classes.appendLine("};");
//  this->classes.appendLine();
//}

std::string PlainCGenerator::strType(const TypeRef& type)
{
  std::string str;

  if (type.id.resolved.type()->typeClass)
    str += type.id.resolved.type()->name;
  else
    str += builtinTypeMapping.at(type.id.str);

  for (int i = 0; i < type.pointerDepth; i++)
    str += "*";

  return str;
}

void PlainCGenerator::referenceType(const TypeRef& typeRef)
{
  if (typeRef.pointerDepth > 0)
    this->usedTypesByRef.emplace(typeRef.id.resolved.type());
  else
    this->usedTypesByValue.emplace(typeRef.id.resolved.type());
}

void PlainCGenerator::referenceFunction(const Func* function)
{
  this->usedFunctions.emplace(function);
  this->referenceType(function->returnType);
  for (const VariableDeclaration* variableDeclaration : function->args)
    this->referenceType(variableDeclaration->type);
}

