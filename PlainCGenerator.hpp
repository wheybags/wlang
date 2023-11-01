#pragma once
#include "Ast.hpp"

class PlainCGenerator
{
public:
  PlainCGenerator();

  std::string generate(const Root* root);

private:
  void generate(const Root* node, int32_t tabIndex);
  void generate(const FuncList* node, int32_t tabIndex);
  void generate(const Func* node, int32_t tabIndex);
  void generate(const Statement* node, std::string& str);
  void generate(const Expression* node, std::string& str);
  void generate(const Class* node);
  std::string strType(const TypeRef& type);

private:
  std::string headers;
  std::string functionPrototypes;
  std::string functionBodies;
  std::string classes;
};


