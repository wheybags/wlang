#pragma once
#include "Ast.hpp"
#include "OutputString.hpp"

class PlainCGenerator
{
public:
  PlainCGenerator();

  std::string output();

  void generate(const Root* root);
  void generate(const Func* node);

private:
  void referenceType(const TypeRef& typeRef);
  void referenceFunction(const Func* function);
  std::string getPrototype(const Func* node);
  void generateClassDeclaration(const Class*, OutputString& str);

  void generate(const FuncList* node);
  void generate(const Block* block, OutputString& str);
  void generate(const Statement* node, OutputString& str);
  std::string generate(const Expression* node);
  std::string generate(const VariableDeclaration* variableDeclaration, bool suppressInitialiser = false);
//  void generate(const Class* node);
  std::string strType(const TypeRef& type);

private:
  std::unordered_set<const Type*> usedTypesByRef;
  std::unordered_set<const Type*> usedTypesByValue;
  std::unordered_set<const Func*> usedFunctions;
  OutputString functionBodies;
};


