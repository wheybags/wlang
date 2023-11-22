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

  void resolveTypeRefs(Root* root);
  void resolveTypeRefs(Class* classN);
  void resolveTypeRefs(Func* func);
  void resolveTypeRefs(Block* block);
  void resolveTypeRefs(Statement* statement);
  void resolveTypeRefs(IfElseChain* ifElseChain);
  void resolveTypeRefs(VariableDeclaration* variableDeclaration);
  void resolveTypeRefs(TypeRef& typeRef);

private:
  std::vector<Scope*> scopeStack;
};