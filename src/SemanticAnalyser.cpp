#include "SemanticAnalyser.hpp"
#include "BuiltinTypes.hpp"
#include "MergedAst.hpp"

void SemanticAnalyser::run(MergedAst& ast)
{
  this->linkScope = &ast.linkScope;

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
  if (!func->external)
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
  release_assert(this->canAssign(assignment->right->type, assignment->left->type));
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

    case Expression::Val::Tag::IntegerConstant:
    {
      expression->type = expression->val.integerConstant().getType()->reference();
      break;
    }

    case Expression::Val::Tag::StringConstant:
    {
      expression->type = this->linkScope->types.at("string").item->reference();
      break;
    }

    case Expression::Val::Tag::Bool:
    {
      expression->type = BuiltinTypes::inst.tBool.reference();
      break;
    }

    case Expression::Val::Tag::Null:
    {
      expression->type = BuiltinTypes::inst.tNull.reference();
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
                         arg->type.id.resolved.type()->builtinNumeric);
          expression->type = BuiltinTypes::inst.tBool.reference();
          break;
        }

        case Op::Type::UnaryMinus:
        {
          Expression* arg = op->args.unary().expression;
          run(arg);
          release_assert(arg->type.pointerDepth == 0 && arg->type.id.resolved.type()->builtinNumeric);
          expression->type = arg->type;
          break;
        }

        case Op::Type::Call:
        {
          Op::Call& callData = op->args.call();

          Func* function = nullptr;
          if (callData.callable->val.isId())
          {
            function = callData.callable->val.id().resolved.function();
            release_assert(callData.callArgs.size() == function->args.size());
          }
          else
          {
            release_assert(callData.callable->val.isOp());
            Op* callOp = callData.callable->val.op();
            release_assert(callOp->type == Op::Type::MemberAccess);
            Expression* object = callOp->args.memberAccess().expression;
            run(object);

            release_assert(object->type.id.resolved.type()->typeClass);
            callOp->args.memberAccess().member.resolveFunction(*object->type.id.resolved.type()->typeClass->memberScope);

            function = callOp->args.memberAccess().member.resolved.function();
            release_assert(callData.callArgs.size() + 1 == function->args.size());
          }

          for (int32_t i = 0; i < int32_t(callData.callArgs.size()); i++)
          {
            run(callData.callArgs[i]);
            release_assert(callData.callArgs[i]->type == function->args[i]->type);
          }

          expression->type = function->returnType;
          break;
        }

        case Op::Type::Subscript:
        {
          Op::Subscript& subscript = op->args.subscript();
          run(subscript.item);
          release_assert(subscript.item->type.pointerDepth > 0);
          run(subscript.index);
          release_assert(subscript.index->type.pointerDepth == 0 && subscript.index->type.id.resolved.type()->builtinNumeric);
          expression->type = subscript.item->type;
          expression->type.pointerDepth--;
          break;
        }

        case Op::Type::MemberAccess:
        {
          Op::MemberAccess& memberAccess = op->args.memberAccess();
          run(memberAccess.expression);
          release_assert(memberAccess.expression->type.pointerDepth <= 1);
          release_assert(memberAccess.expression->type.id.resolved.type()->typeClass);

          // As an exception, we resolve this here, as it depends on fetching the scope of the actual type, which is not available during
          // the normal resolveScopeIds pass.
          memberAccess.member.resolveVariableDeclaration(*memberAccess.expression->type.id.resolved.type()->typeClass->memberScope);
          release_assert(memberAccess.member.resolved.variableDeclaration());

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
          release_assert(binary.left->type.pointerDepth == 0 && binary.left->type.id.resolved.type()->builtinNumeric);
          release_assert(binary.right->type.pointerDepth == 0 && binary.right->type.id.resolved.type()->builtinNumeric);
          expression->type = BuiltinTypes::resolveBinaryOperatorPromotion(binary.left->type.id.resolved.type(), binary.right->type.id.resolved.type())->reference();
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
    release_assert(this->canAssign(variableDeclaration->initialiser->type, variableDeclaration->type));
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

  for (Func* func : root->funcList->functions)
  {
    if (!func->external)
      resolveScopeIds(func->funcBody);
  }

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

    case Expression::Val::Tag::IntegerConstant:
    case Expression::Val::Tag::Bool:
    case Expression::Val::Tag::StringConstant:
    case Expression::Val::Tag::Null:
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
          if (call.callable->val.isId())
          {
            call.callable->val.id().resolveFunction(*scopeStack.back());
            release_assert(call.callable->val.id().resolved.function());
          }
          else
          {
            release_assert(call.callable->val.isOp() && call.callable->val.op()->type == Op::Type::MemberAccess);
            resolveScopeIds(call.callable->val.op()->args.memberAccess().expression);
            // Cannot resolve the member function name yet, see case Op::Type::MemberAccess below
          }

          for (int32_t i = 0; i < int32_t(call.callArgs.size()); i++)
            resolveScopeIds(call.callArgs[i]);
          break;
        }

        case Op::Type::Subscript:
        {
          Op::Subscript& subscript = op->args.subscript();
          resolveScopeIds(subscript.item);
          resolveScopeIds(subscript.index);
          break;
        }

        case Op::Type::MemberAccess:
        {
          Op::MemberAccess& memberAccess = op->args.memberAccess();
          resolveScopeIds(memberAccess.expression);
          // Cannot resolve the member name yet, as it depends on the derived type of the MemberAccess::expression expression.
          // So, as an exception, this is deferred to the next pass.
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

bool SemanticAnalyser::canAssign(const TypeRef& source, const TypeRef& destination)
{
  if (source == destination)
    return true;

  if (destination.pointerDepth > 0 && source == BuiltinTypes::inst.tNull.reference())
    return true;

  return false;
}

