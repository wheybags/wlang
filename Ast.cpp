#include "Ast.hpp"

template<typename T>
T* Scope::lookup(std::string_view name)
{
  HashMap<Item<T*>>* map = nullptr;
  if constexpr (std::is_same<T, Func>::value)
    map = &this->functions;
  if constexpr (std::is_same<T, VariableDeclaration>::value)
    map = &this->variables;
  if constexpr (std::is_same<T, Type>::value)
    map = &this->types;

  auto it = map->find(name);
  if (it != map->end())
    return it->second.item;

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

template Func* Scope::lookup<Func>(std::string_view name);
template VariableDeclaration* Scope::lookup<VariableDeclaration>(std::string_view name);
template Type* Scope::lookup<Type>(std::string_view name);


bool TypeRef::operator==(TypeRef& other)
{
  return pointerDepth == other.pointerDepth && (typeResolved == other.typeResolved || (typeResolved && other.typeResolved && typeResolved->typeClass && typeResolved->typeClass == other.typeResolved->typeClass));
}

bool TypeRef::operator!=(TypeRef& other)
{
  return !(*this == other);
}