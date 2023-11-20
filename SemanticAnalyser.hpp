#pragma once
#include "Ast.hpp"

class MergedAst;

class SemanticAnalyser
{
public:
  void run(MergedAst& ast);
  void run(Root* root);

private:
  void run(Class* classN);
  void run(Func* func);
  void run(Block* block, Func* func);
  void run(Statement* statement, Func* func);
  void run(Expression* expression);
  void run(Assignment* assignment);
  void run(VariableDeclaration* variableDeclaration);
  void run(ReturnStatement* returnStatement, Func* func);
  void run(IfElseChain* ifElseChain, Func* func);

private:
  std::vector<Scope*> scopeStack;
};