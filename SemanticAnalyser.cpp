#pragma once
#include "SemanticAnalyser.hpp"
#include "BuiltinTypes.hpp"
#include "MergedAst.hpp"

void SemanticAnalyser::run(MergedAst& ast)
{
  for (AstChunk* chunk : ast)
    resolveScopeIds(chunk->root);

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
  for (VariableDeclaration* variableDeclaration : classN->memberVariables)
  {
    run(variableDeclaration);
    // TODO: recursive check
    release_assert(variableDeclaration->type != classN->type->reference());
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
      VariableDeclaration* var = expression->val.id().resolved.variableDeclaration();
      if (!expression->source.isAfterEndOf(var->source))
      {
        message_and_abort_fmt("%s (%d:%d) used before definition (%d:%d)",
                              expression->val.id().str.c_str(),
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
          Op::Binary& binary = op->args.binary();
          run(binary.left);
          run(binary.right);
          release_assert(binary.left->type == binary.right->type);
          expression->type = BuiltinTypes::inst.tBool.reference();
          break;
        }

        case Op::Type::LogicalNot:
        {
          Expression* arg = op->args.unary().expression;
          run(arg);
          release_assert(arg->type.pointerDepth > 0 ||
                         arg->type.id.resolved.type() == &BuiltinTypes::inst.tBool ||
                         arg->type.id.resolved.type() == &BuiltinTypes::inst.tI32);
          expression->type = BuiltinTypes::inst.tBool.reference();
          break;
        }

        case Op::Type::UnaryMinus:
        {
          Expression* arg = op->args.unary().expression;
          run(arg);
          release_assert(arg->type == BuiltinTypes::inst.tI32.reference());
          expression->type = BuiltinTypes::inst.tI32.reference();
          break;
        }

        case Op::Type::Call:
        {
          Op::Call& callData = op->args.call();
          Func* function = callData.callable->val.id().resolved.function();

          release_assert(callData.callArgs.size() == function->args.size());
          for (int32_t i = 0; i < int32_t(callData.callArgs.size()); i++)
          {
            run(callData.callArgs[i]);
            release_assert(callData.callArgs[i]->type == function->args[i]->type);
          }

          expression->type = function->returnType;
          break;
        }

        case Op::Type::MemberAccess:
        {
          Op::MemberAccess& memberAccess = op->args.memberAccess();
          run(memberAccess.expression);
          release_assert(memberAccess.expression->type.pointerDepth <= 1);
          release_assert(memberAccess.expression->type.id.resolved.type()->typeClass);
          expression->type = memberAccess.member.resolved.variableDeclaration()->type;
          break;
        }

        case Op::Type::Add:
        case Op::Type::Subtract:
        case Op::Type::Multiply:
        case Op::Type::Divide:
        {
          Op::Binary& binary = op->args.binary();
          run(binary.left);
          run(binary.right);
          release_assert(binary.left->type == BuiltinTypes::inst.tI32.reference());
          release_assert(binary.right->type == BuiltinTypes::inst.tI32.reference());
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

  release_assert(expression->type.id.resolved.type());
}

void SemanticAnalyser::run(VariableDeclaration* variableDeclaration)
{
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

void SemanticAnalyser::resolveScopeIds(Root* root)
{
  this->scopeStack.emplace_back(root->funcList->scope);
  for (Func* func : root->funcList->functions)
    resolveScopeIds(func);
  for (Class* classN : root->funcList->classes)
    resolveScopeIds(classN);
  this->scopeStack.resize(this->scopeStack.size()-1);
}

void SemanticAnalyser::resolveScopeIds(Class* classN)
{
  for (VariableDeclaration* variableDeclaration : classN->memberVariables)
    resolveScopeIds(variableDeclaration);
}

void SemanticAnalyser::resolveScopeIds(Func* func)
{
  resolveScopeIds(func->returnType);
  for (VariableDeclaration* arg : func->args)
    resolveScopeIds(arg);
  resolveScopeIds(func->funcBody);
}

void SemanticAnalyser::resolveScopeIds(Block* block)
{
  this->scopeStack.emplace_back(block->scope);
  for (Statement* statement : block->statements)
    resolveScopeIds(statement);
  this->scopeStack.resize(this->scopeStack.size()-1);
}

void SemanticAnalyser::resolveScopeIds(Statement* statement)
{
  switch (statement->tag())
  {
    case Statement::Tag::Return:
      resolveScopeIds(statement->returnStatment());
      break;
    case Statement::Tag::Variable:
      resolveScopeIds(statement->variable());
      break;
    case Statement::Tag::Assignment:
      resolveScopeIds(statement->assignment());
      break;
    case Statement::Tag::Expression:
      resolveScopeIds(statement->expression());
      break;
    case Statement::Tag::IfElseChain:
      resolveScopeIds(statement->ifElseChain());
      break;
    case Statement::Tag::None:
      message_and_abort("bad statement");
  }
}

void SemanticAnalyser::resolveScopeIds(IfElseChain* ifElseChain)
{
  for (int32_t i = 0; i < int32_t(ifElseChain->items.size()); i++)
  {
    IfElseChainItem* item = ifElseChain->items[i];
    resolveScopeIds(item->block);
  }
}

void SemanticAnalyser::resolveScopeIds(VariableDeclaration* variableDeclaration)
{
  resolveScopeIds(variableDeclaration->type);
  if (variableDeclaration->initialiser)
    resolveScopeIds(variableDeclaration->initialiser);
}

void SemanticAnalyser::resolveScopeIds(TypeRef& typeRef)
{
  typeRef.id.resolveType(*scopeStack.back());
  release_assert(typeRef.id.resolved.type());
}

void SemanticAnalyser::resolveScopeIds(Expression* expression)
{
  switch (expression->val.tag())
  {
    case Expression::Val::Tag::Id:
    {
      expression->val.id().resolveVariableDeclaration(*scopeStack.back());
      release_assert(expression->val.id().resolved.variableDeclaration());
      break;
    }

    case Expression::Val::Tag::Int32:
    case Expression::Val::Tag::Bool:
      break;

    case Expression::Val::Tag::Op:
    {
      Op* op = expression->val.op();
      switch (op->type)
      {
        case Op::Type::CompareEqual:
        case Op::Type::CompareNotEqual:
        case Op::Type::LogicalAnd:
        case Op::Type::LogicalOr:
        case Op::Type::Add:
        case Op::Type::Subtract:
        case Op::Type::Multiply:
        case Op::Type::Divide:
        {
          Op::Binary& binary = op->args.binary();
          resolveScopeIds(binary.left);
          resolveScopeIds(binary.right);
          break;
        }

        case Op::Type::UnaryMinus:
        case Op::Type::LogicalNot:
        {
          resolveScopeIds(op->args.unary().expression);
          break;
        }

        case Op::Type::Call:
        {
          Op::Call& call = op->args.call();
          release_assert(call.callable->val.isId()); // TODO: this doesn't belong here. Fix when implementing function pointers
          call.callable->val.id().resolveFunction(*scopeStack.back());
          release_assert(call.callable->val.id().resolved.function());
          for (int32_t i = 0; i < int32_t(call.callArgs.size()); i++)
            resolveScopeIds(call.callArgs[i]);
          break;
        }

        case Op::Type::MemberAccess:
        {
          Op::MemberAccess& memberAccess = op->args.memberAccess();
          resolveScopeIds(memberAccess.expression);
          memberAccess.member.resolveVariableDeclaration(*scopeStack.back());
          release_assert(memberAccess.member.resolved.variableDeclaration());
          break;
        }

        case Op::Type::ENUM_END:
          message_and_abort("empty op!");
      }
      break;
    }

    case Expression::Val::Tag::None:
      message_and_abort("empty expression!");

  }
}

void SemanticAnalyser::resolveScopeIds(Assignment* assignment)
{
  resolveScopeIds(assignment->left);
  resolveScopeIds(assignment->right);
}

void SemanticAnalyser::resolveScopeIds(ReturnStatement* returnStatement)
{
  resolveScopeIds(returnStatement->retval);
}

