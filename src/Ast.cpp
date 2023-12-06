#include "Ast.hpp"

template<typename T>
T* Scope::lookup(ScopeId& name)
{
  HashMap<Item<T*>>* map = nullptr;
  if constexpr (std::is_same<T, Func>::value)
    map = &this->functions;
  if constexpr (std::is_same<T, VariableDeclaration>::value)
    map = &this->variables;
  if constexpr (std::is_same<T, Type>::value)
    map = &this->types;

  auto it = map->find(name.str);
  if (it != map->end())
  {
    name.resolved = it->second.item;
    return it->second.item;
  }

  if (this->parent2)
  {
    T* item = this->parent2->lookup<T>(name);
    if (item)
      return item;
  }

  if (this->parent)
  {
    T* item = this->parent->lookup<T>(name);
    if (item)
      return item;
  }

  return nullptr;
}

template Func* Scope::lookup<Func>(ScopeId& name);
template VariableDeclaration* Scope::lookup<VariableDeclaration>(ScopeId& name);
template Type* Scope::lookup<Type>(ScopeId& name);


bool TypeRef::operator==(TypeRef& other)
{
  if (pointerDepth != other.pointerDepth)
    return false;

  if (id.resolved == other.id.resolved)
    return true;

  if (id.resolved &&
      other.id.resolved &&
      id.resolved.type()->typeClass &&
      id.resolved.type()->typeClass == other.id.resolved.type()->typeClass)
  {
    return true;
  }

  return false;
}

bool TypeRef::operator!=(TypeRef& other)
{
  return !(*this == other);
}

void ScopeId::resolveFunction(Scope& scope)
{
  scope.lookup<Func>(*this);
}

void ScopeId::resolveVariableDeclaration(Scope& scope)
{
  scope.lookup<VariableDeclaration>(*this);
}

void ScopeId::resolveType(Scope& scope)
{
  scope.lookup<Type>(*this);
}
