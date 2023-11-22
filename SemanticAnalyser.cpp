#pragma once
#include "SemanticAnalyser.hpp"
#include "BuiltinTypes.hpp"
#include "MergedAst.hpp"

void SemanticAnalyser::run(MergedAst& ast)
{
  for (AstChunk* chunk : ast)
    resolveTypeRefs(chunk->root);

  for (AstChunk* chunk : ast)
    run(chunk->root);
}

void SemanticAnalyser::run(Root* root)
{
  this->scopeStack.emplace_back(root->funcList->scope);
  for (Func* func : root->funcList->functions)
    run(func);
  for (Class* classN : root->funcList->classes)
    run(classN);
  this->scopeStack.resize(this->scopeStack.size()-1);
}

void SemanticAnalyser::run(Func* func)
{
  release_assert(func->returnType.typeResolved->defined());
  run(func->funcBody, func);
}

void SemanticAnalyser::run(Block* block, Func* func)
{
  this->scopeStack.emplace_back(block->scope);
  for (Statement* statement : block->statements)
    run(statement, func);
  this->scopeStack.resize(this->scopeStack.size()-1);
}

void SemanticAnalyser::run(Class* classN)
{
  for (auto& pair : classN->memberVariables)
  {
    VariableDeclaration* variableDeclaration = pair.second;
    run(variableDeclaration);
    release_assert(variableDeclaration->type.typeResolved != classN->type || variableDeclaration->type.pointerDepth > 0);
  }
}

void SemanticAnalyser::run(Statement* statement, Func* func)
{
  switch (statement->tag())
  {
    case Statement::Tag::Return:
      run(statement->returnStatment(), func);
      break;
    case Statement::Tag::Variable:
      run(statement->variable());
      break;
    case Statement::Tag::Assignment:
      run(statement->assignment());
      break;
    case Statement::Tag::Expression:
      run(statement->expression());
      break;
    case Statement::Tag::IfElseChain:
      run(statement->ifElseChain(), func);
      break;
    case Statement::Tag::None:
      message_and_abort("bad statement");
  }
}

void SemanticAnalyser::run(Assignment* assignment)
{
  run(assignment->left);
  run(assignment->right);
  release_assert(assignment->left->type == assignment->right->type);
}

void SemanticAnalyser::run(Expression* expression)
{
  switch (expression->val.tag())
  {
    case Expression::Val::Tag::Id:
    {
      VariableDeclaration* var = this->scopeStack.back()->lookupVar(expression->val.id());
      release_assert(var);

      if (!expression->source.isAfterEndOf(var->source))
      {
        message_and_abort_fmt("%s (%d:%d) used before definition (%d:%d)",
                              expression->val.id().c_str(),
                              expression->source.start.y, expression->source.start.x,
                              var->source.start.y, var->source.start.x);
      }

      expression->type = var->type;
      break;
    }

    case Expression::Val::Tag::Int32:
    {
      expression->type = BuiltinTypes::inst.tI32.reference();
      break;
    }

    case Expression::Val::Tag::Bool:
    {
      expression->type = BuiltinTypes::inst.tBool.reference();
      break;
    }

    case Expression::Val::Tag::Op:
    {
      Op* op = expression->val.op();
      switch (op->type)
      {
        case Op::Type::CompareEqual:
        case Op::Type::CompareNotEqual:
        case Op::Type::LogicalAnd:
        case Op::Type::LogicalOr:
        {
          run(op->left);
          run(op->right);
          release_assert(op->left->type == op->right->type);
          expression->type = BuiltinTypes::inst.tBool.reference();
          break;
        }

        case Op::Type::LogicalNot:
        {
          run(op->left);
          release_assert(op->left->type.pointerDepth > 0 ||
                         op->left->type.typeResolved == &BuiltinTypes::inst.tBool ||
                         op->left->type.typeResolved == &BuiltinTypes::inst.tI32);
          expression->type = BuiltinTypes::inst.tBool.reference();
          break;
        }

        case Op::Type::UnaryMinus:
        {
          run(op->left);
          release_assert(op->left->type.typeResolved == &BuiltinTypes::inst.tI32);
          expression->type = BuiltinTypes::inst.tI32.reference();
          break;
        }

        case Op::Type::Call:
        {
          release_assert(op->left->val.isId());

          Func* function = this->scopeStack.back()->lookupFunc(op->left->val.id());
          release_assert(function);

          release_assert(op->callArgs.size() == function->args.size());
          for (int32_t i = 0; i < int32_t(op->callArgs.size()); i++)
          {
            run(op->callArgs[i]);
            release_assert(op->callArgs[i]->type == function->args[i]->type);
          }

          expression->type = function->returnType;
          break;
        }

        case Op::Type::MemberAccess:
        {
          run(op->left);
          release_assert(op->right->val.isId());

          release_assert(op->left->type.pointerDepth <= 1);
          release_assert(op->left->type.typeResolved->typeClass);

          auto it = op->left->type.typeResolved->typeClass->memberVariables.find(op->right->val.id());
          release_assert(it != op->left->type.typeResolved->typeClass->memberVariables.end());

          expression->type = it->second->type;
          break;
        }

        case Op::Type::Add:
        case Op::Type::Subtract:
        case Op::Type::Multiply:
        case Op::Type::Divide:
        {
          run(op->left);
          run(op->right);
          release_assert(op->left->type == BuiltinTypes::inst.tI32.reference());
          release_assert(op->right->type == BuiltinTypes::inst.tI32.reference());
          expression->type = BuiltinTypes::inst.tI32.reference();
          break;
        }
        case Op::Type::ENUM_END:
          break;
      }
      break;
    }

    case Expression::Val::Tag::None:
      message_and_abort("empty expression!");
  }

  release_assert(expression->type.typeResolved);
}

void SemanticAnalyser::run(VariableDeclaration* variableDeclaration)
{
  release_assert(variableDeclaration->type.typeResolved);
  release_assert(variableDeclaration->type.typeResolved->defined());

  if (variableDeclaration->initialiser)
  {
    run(variableDeclaration->initialiser);
    release_assert(variableDeclaration->initialiser->type == variableDeclaration->type);
  }
}

void SemanticAnalyser::run(ReturnStatement* returnStatement, Func* func)
{
  run(returnStatement->retval);
  release_assert(returnStatement->retval->type == func->returnType);
}

void SemanticAnalyser::run(IfElseChain* ifElseChain, Func* func)
{
  for (int32_t i = 0; i < int32_t(ifElseChain->items.size()); i++)
  {
    IfElseChainItem* item = ifElseChain->items[i];

    if (item->condition)
    {
      run(item->condition);
      release_assert(item->condition->type == BuiltinTypes::inst.tBool.reference());
    }
    else
    {
      release_assert(ifElseChain->items.size() > 1 && i == int32_t(ifElseChain->items.size()) - 1);
    }

    run(item->block, func);
  }
}

void SemanticAnalyser::resolveTypeRefs(Root* root)
{
  this->scopeStack.emplace_back(root->funcList->scope);
  for (Func* func : root->funcList->functions)
    resolveTypeRefs(func);
  for (Class* classN : root->funcList->classes)
    resolveTypeRefs(classN);
  this->scopeStack.resize(this->scopeStack.size()-1);
}

void SemanticAnalyser::resolveTypeRefs(Class* classN)
{
  for (auto& pair : classN->memberVariables)
  {
    VariableDeclaration* variableDeclaration = pair.second;
    resolveTypeRefs(variableDeclaration);
  }
}

void SemanticAnalyser::resolveTypeRefs(Func* func)
{
  resolveTypeRefs(func->returnType);
  for (VariableDeclaration* arg : func->args)
    resolveTypeRefs(arg);
  resolveTypeRefs(func->funcBody);
}

void SemanticAnalyser::resolveTypeRefs(Block* block)
{
  this->scopeStack.emplace_back(block->scope);
  for (Statement* statement : block->statements)
    resolveTypeRefs(statement);
  this->scopeStack.resize(this->scopeStack.size()-1);
}

void SemanticAnalyser::resolveTypeRefs(Statement* statement)
{
  switch (statement->tag())
  {
    case Statement::Tag::IfElseChain:
      resolveTypeRefs(statement->ifElseChain());
      break;
    case Statement::Tag::Variable:
      resolveTypeRefs(statement->variable());
      break;
    case Statement::Tag::Assignment:
    case Statement::Tag::Expression:
    case Statement::Tag::Return:
      break;
    case Statement::Tag::None:
      message_and_abort("bad statement");
  }
}

void SemanticAnalyser::resolveTypeRefs(IfElseChain* ifElseChain)
{
  for (int32_t i = 0; i < int32_t(ifElseChain->items.size()); i++)
  {
    IfElseChainItem* item = ifElseChain->items[i];
    resolveTypeRefs(item->block);
  }
}

void SemanticAnalyser::resolveTypeRefs(VariableDeclaration* variableDeclaration)
{
  resolveTypeRefs(variableDeclaration->type);
}

void SemanticAnalyser::resolveTypeRefs(TypeRef& typeRef)
{
  typeRef.typeResolved = scopeStack.back()->lookupType(typeRef.typeName);
  release_assert(typeRef.typeResolved);
}

