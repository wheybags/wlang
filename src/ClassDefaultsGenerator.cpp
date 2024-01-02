#include "ClassDefaultsGenerator.hpp"
#include "AstChunk.hpp"
#include "BuiltinTypes.hpp"

void generateClassDefaults(AstChunk& ast)
{
  for (Class* classN : ast.root->funcList->classes)
  {
    Func* func = ast.makeNode<Func>();
    func->argsScope = ast.makeNode<Scope>();
    {
      VariableDeclaration* thisDeclaration = ast.makeNode<VariableDeclaration>();
      thisDeclaration->name = "this";
      thisDeclaration->type = classN->type->reference();
      thisDeclaration->type.pointerDepth = 1;

      func->args.emplace_back(thisDeclaration);
      func->argsScope->variables.insert_or_assign(thisDeclaration->name, Scope::Item<VariableDeclaration*>{.item = thisDeclaration, .chunk = &ast});
    }


    func->returnType = BuiltinTypes::inst.tI32.reference(); // TODO: replace when I add void
    func->name = "defaultConstruct";

    {
      Block* block = ast.makeNode<Block>();
      block->scope = ast.makeNode<Scope>();
      block->scope->parent = ast.root->funcList->scope;

      for (VariableDeclaration* variableDeclaration : classN->memberVariables)
      {
        if (variableDeclaration->initialiser)
        {
          Statement* statement = ast.makeNode<Statement>();
          Assignment* assignment = ast.makeNode<Assignment>();

          assignment->left = ast.makeNode<Expression>();
          {
            Op* op = ast.makeNode<Op>();
            op->type = Op::Type::MemberAccess;

            Expression* thisExpression = ast.makeNode<Expression>();
            thisExpression->val = ScopeId("this");

            op->args = Op::MemberAccess{.expression = thisExpression, .member = ScopeId(variableDeclaration->name)};
            assignment->left->val = op;
          }

          assignment->right = variableDeclaration->initialiser;

          *statement = assignment;
          block->statements.emplace_back(statement);
        }
      }

      func->funcBody = block;
    }

    func->funcBody->scope->parent2 = func->argsScope;

    ast.root->funcList->functions.emplace_back(func);

    classN->memberScope->functions.insert_or_assign(func->name, Scope::Item<Func*>{.item = func, .chunk = &ast});
    func->memberClass = classN;
  }
}