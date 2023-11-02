#pragma once
#include "SemanticAnalyser.hpp"
#include "Parser.hpp"

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
  for (auto& pair : classN->memberVariables)
  {
    VariableDeclaration* variableDeclaration = pair.second;
    run(variableDeclaration);
    release_assert(variableDeclaration->type.type != classN->type || variableDeclaration->type.pointerDepth > 0);
  }
}

void SemanticAnalyser::run(Statement* statement, Func* func)
{
  if (std::holds_alternative<ReturnStatement*>(*statement))
    run(std::get<ReturnStatement*>(*statement), func);
  else if (std::holds_alternative<VariableDeclaration*>(*statement))
    run(std::get<VariableDeclaration*>(*statement));
  else if (std::holds_alternative<Assignment*>(*statement))
    run(std::get<Assignment*>(*statement));
  else if (std::holds_alternative<Expression*>(*statement))
    run(std::get<Expression*>(*statement));
  else if (std::holds_alternative<IfElseChain*>(*statement))
    run(std::get<IfElseChain*>(*statement), func);
  else
    message_and_abort("bad statement");
}

void SemanticAnalyser::run(Assignment* assignment)
{
  run(assignment->left);
  run(assignment->right);
  release_assert(assignment->left->type == assignment->right->type);
}

void SemanticAnalyser::run(Expression* expression)
{
  switch (expression->tag())
  {
    case Expression::Id:
    {
      ScopeItem item = this->scopeStack.back()->lookup(expression->id());
      release_assert(item);
      release_assert(item.isVariable());
      expression->type = item.variable()->type;
      break;
    }

    case Expression::Int32:
    {
      expression->type = { .type = this->parser.tInt32 };
      break;
    }

    case Expression::Bool:
    {
      expression->type = { .type = this->parser.tBool };
      break;
    }

    case Expression::Op:
    {
      Op* op = expression->op();
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
          expression->type = { .type = this->parser.tBool };
          break;
        }

        case Op::Type::LogicalNot:
        {
          run(op->left);
          release_assert(op->left->type.pointerDepth > 0 ||
                         op->left->type.type == this->parser.tBool ||
                         op->left->type.type == this->parser.tInt32);
          expression->type = { .type = this->parser.tBool };
          break;
        }

        case Op::Type::UnaryMinus:
        {
          run(op->left);
          release_assert(op->left->type.type == this->parser.tInt32);
          expression->type = { .type = this->parser.tInt32 };
          break;
        }

        case Op::Type::Call:
        {
          release_assert(op->left->isId());

          ScopeItem item = this->scopeStack.back()->lookup(op->left->id());
          release_assert(item);
          release_assert(item.isFunction());

          Func* function = item.function();

          release_assert(op->callArgs.size() == function->argsOrder.size());
          for (int32_t i = 0; i < int32_t(op->callArgs.size()); i++)
          {
            run(op->callArgs[i]);

            const std::string& argName = function->argsOrder[i];
            release_assert(op->callArgs[i]->type == function->argsScope->lookup(argName).variable()->type);
          }

          expression->type = function->returnType;
          break;
        }

        case Op::Type::MemberAccess:
        {
          run(op->left);
          release_assert(op->right->isId());

          release_assert(op->left->type.pointerDepth <= 1);
          release_assert(op->left->type.type->typeClass);

          auto it = op->left->type.type->typeClass->memberVariables.find(op->right->id());
          release_assert(it != op->left->type.type->typeClass->memberVariables.end());

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
          release_assert(op->left->type == TypeRef{ .type = this->parser.tInt32 });
          release_assert(op->right->type == TypeRef{ .type = this->parser.tInt32 });
          expression->type = { .type = this->parser.tInt32 };
          break;
        }
        case Op::Type::ENUM_END:
          break;
      }
      break;
    }

    case Expression::None:
      message_and_abort("empty expression!");
  }

  release_assert(expression->type.type);
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
      release_assert(item->condition->type == TypeRef{ .type = this->parser.tBool });
    }
    else
    {
      release_assert(ifElseChain->items.size() > 1 && i == int32_t(ifElseChain->items.size()) - 1);
    }

    run(item->block, func);
  }
}

