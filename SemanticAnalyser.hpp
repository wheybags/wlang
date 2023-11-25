#pragma once
#include "Ast.hpp"

class MergedAst;

class SemanticAnalyser
{
public:
  void run(MergedAst& ast);

private:
  void run(Root* root);
  void run(Class* classN);
  void run(Func* func);
  void run(Block* block, Func* func);
  void run(Statement* statement, Func* func);
  void run(Expression* expression);
  void run(Assignment* assignment);
  void run(VariableDeclaration* variableDeclaration);
  void run(ReturnStatement* returnStatement, Func* func);
  void run(IfElseChain* ifElseChain, Func* func);

  void resolveScopeIds(Root* root);
  void resolveScopeIds(Class* classN);
  void resolveScopeIds(Func* func);
  void resolveScopeIds(Block* block);
  void resolveScopeIds(Statement* statement);
  void resolveScopeIds(Expression* expression);
  void resolveScopeIds(Assignment* assignment);
  void resolveScopeIds(IfElseChain* ifElseChain);
  void resolveScopeIds(VariableDeclaration* variableDeclaration);
  void resolveScopeIds(ReturnStatement* returnStatement);
  void resolveScopeIds(TypeRef& typeRef);

private:
  std::vector<Scope*> scopeStack;
};