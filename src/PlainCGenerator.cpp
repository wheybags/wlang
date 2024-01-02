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
    if (type->builtin)
      return;

    release_assert(!seenThisIteration.contains(type)); // circular reference!

    bool wasSeen = seen.contains(type);
    seen.insert(type);
    seenThisIteration.insert(type);

    if (type->typeClass)
    {
      for (const VariableDeclaration* member: type->typeClass->memberVariables)
      {
        if (member->type.id.resolved.type()->builtin)
          continue;

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

  OutputString stringConstantsOutput;
  for (const auto& pair : this->stringConstants)
  {
    std::string charArrayName = pair.second + "_charArray";
    stringConstantsOutput.appendLine("static const char " + charArrayName + "[] = " + pair.first + ";");
    stringConstantsOutput.appendLine("static struct string " + pair.second + " = { .data = (char*)" + charArrayName + ", .length = sizeof(" + charArrayName + ") - 1 };");
  }

  return declarations.str + stringConstantsOutput.str + functionBodies.str;
}

void PlainCGenerator::generate(const Root *root)
{
  generate(root->funcList);
}

static const std::unordered_map<std::string, std::string> builtinTypeMapping
{
  {"i8", "char"},
  {"i16", "short"},
  {"i32", "int"},
  {"i64", "long long int"},
  {"bool", "char"}
};

void PlainCGenerator::generate(const FuncList* node)
{
  for (Func* func : node->functions)
    generate(func);
}

std::string PlainCGenerator::getPrototype(const Func* node)
{
  std::string prototype= strType(node->returnType) + " " + node->mangledName + "(";

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
  if (node->external)
    return;

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
      generate(node->variable(), str);
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

void PlainCGenerator::generate(const VariableDeclaration* variableDeclaration, OutputString& str, bool suppressInitialiser)
{
  this->referenceType(variableDeclaration->type);

  Class* typeClass = variableDeclaration->type.id.resolved.type()->typeClass;

  std::string line;
  if (typeClass)
  {
    release_assert(!variableDeclaration->initialiser && "not supported yet");
    line += strType(variableDeclaration->type) + " " + variableDeclaration->name;// + "; ";
  }
  else
  {
    line += strType(variableDeclaration->type) + " " + variableDeclaration->name;
  }

  std::string line2;
  if (!suppressInitialiser)
  {
    if (variableDeclaration->initialiser)
    {
      line += " = ";
      line += generate(variableDeclaration->initialiser);
    }
    else if (variableDeclaration->type.pointerDepth == 0 && variableDeclaration->type.id.resolved.type()->typeClass)
    {
      const Func* defaultConstructor = variableDeclaration->type.id.resolved.type()->typeClass->memberScope->functions.at("defaultConstruct").item;
      line2 = defaultConstructor->mangledName + "(&" + variableDeclaration->name + ")";
      this->referenceFunction(defaultConstructor);
    }
  }

  str.appendLine(line + ";");
  if (!line2.empty())
    str.appendLine(line2 + ";");
}

std::string PlainCGenerator::generate(const Expression* node)
{
  std::string str;

  switch(node->val.tag())
  {
    case Expression::Val::Tag::Id:
    {
      const ScopeId& scopeId = node->val.id();
      str += scopeId.str;
      break;
    }

    case Expression::Val::Tag::IntegerConstant:
    {
      const IntegerConstant& integerConstant = node->val.integerConstant();
      switch (integerConstant.size)
      {
        case 8:
          str += "((char)" + std::to_string(integerConstant.val) + ")";
          break;
        case 16:
          str += "((short)" + std::to_string(integerConstant.val) + ")";
          break;
        case 32:
          str += std::to_string(integerConstant.val);
          break;
        case 64:
          str += std::to_string(integerConstant.val) + "LL";
          break;
        default:
          message_and_abort("bad integer size");
      }
      break;
    }

    case Expression::Val::Tag::StringConstant:
    {
      const std::string& string = node->val.stringConstant().val;
      auto it = this->stringConstants.find(string);
      if (it == this->stringConstants.end())
        it = this->stringConstants.insert_or_assign(string, "str_" + std::to_string(this->stringConstants.size())).first;
      str += it->second;
      break;
    }

    case Expression::Val::Tag::Bool:
    {
      str += node->val.boolean() ? "1" : "0";
      break;
    }

    case Expression::Val::Tag::Null:
    {
      str += "0";
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
          TypeRef parentType = memberAccess.expression->type;
          parentType.pointerDepth--;
          this->referenceType(parentType);
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
        case Op::Type::AddressOf:
        {
          str += "(&";
          str += generate(opNode->args.unary().expression);
          str += ")";
          break;
        }
        case Op::Type::Call:
        {
          const Op::Call& call = opNode->args.call();
          if (call.callable->val.isId())
          {
            const Func* function = call.callable->val.id().resolved.function();
            this->referenceFunction(function);
            str += "((";
            str += function->mangledName;
            str += ")(";
            for (int32_t i = 0; i < int32_t(call.callArgs.size()); i++)
            {
              str += generate(call.callArgs[i]);
              if (i != int32_t(call.callArgs.size()) - 1)
                str += ", ";
            }
            str += "))";
          }
          else
          {
            // is member call
            const Op* callOp = call.callable->val.op();
            this->referenceFunction(callOp->args.memberAccess().member.resolved.function());

            str += "("+ callOp->args.memberAccess().member.resolved.function()->mangledName + "(";

            if (callOp->args.memberAccess().expression->type.pointerDepth == 0)
              str += "&";
            str += "(" + generate(callOp->args.memberAccess().expression) + ")";

            if (!call.callArgs.empty())
              str += ", ";
            for (int32_t i = 0; i < int32_t(call.callArgs.size()); i++)
            {
              str += generate(call.callArgs[i]);
              if (i != int32_t(call.callArgs.size()) - 1)
                str += ", ";
            }
            str += "))";
          }
          break;
        }
        case Op::Type::Subscript:
        {
          const Op::Subscript& subscript = opNode->args.subscript();
          str += "((";
          str += generate(subscript.item);
          str += ")";
          str += "[";
          str += generate(subscript.index);
          str += "])";
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
    generate(variableDeclaration, str, true);

  str.appendLine("};");
  str.appendLine();
}

std::string PlainCGenerator::strType(const TypeRef& type)
{
  std::string str;

  if (type.id.resolved.type()->typeClass)
    str += "struct " + type.id.resolved.type()->name;
  else
    str += builtinTypeMapping.at(type.id.str);

  for (int i = 0; i < type.pointerDepth; i++)
    str += "*";

  return str;
}

void PlainCGenerator::referenceType(const TypeRef& typeRef)
{
  if (typeRef.id.resolved.type()->builtin)
    return;

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

