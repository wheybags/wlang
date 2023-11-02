#pragma once
#include "Ast.hpp"
#include "OutputString.hpp"

class PlainCGenerator
{
public:
  PlainCGenerator();

  std::string generate(const Root* root);

private:
  void generate(const FuncList* node);
  void generate(const Func* node);
  void generate(const Block* block, OutputString& str);
  void generate(const Statement* node, OutputString& str);
  std::string generate(const Expression* node);
  std::string generate(const VariableDeclaration* variableDeclaration);
  void generate(const Class* node);
  std::string strType(const TypeRef& type);

private:
  OutputString headers;
  OutputString functionPrototypes;
  OutputString functionBodies;
  OutputString classes;
};


