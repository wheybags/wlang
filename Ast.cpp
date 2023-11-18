#include "Ast.hpp"

ScopeItem Scope::lookup(const std::string& name)
{
  auto it = this->variables.find(name);
  if (it != this->variables.end())
    return it->second;

  if (this->parent2)
  {
    ScopeItem item = this->parent2->lookup(name);
    if (item)
      return item;
  }

  if (this->parent)
  {
    ScopeItem item = this->parent->lookup(name);
    if (item)
      return item;
  }

  return {};
}