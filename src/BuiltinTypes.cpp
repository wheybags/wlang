#include "BuiltinTypes.hpp"

BuiltinTypes BuiltinTypes::inst;

BuiltinTypes::BuiltinTypes()
{
  Type* it = (&tI32);
  while (it <= &tBool)
  {
    this->typeMap.insert_or_assign(it->name, it);
    it++;
  }
}

Type* BuiltinTypes::get(std::string_view name)
{
  auto it = this->typeMap.find(name);
  return it == this->typeMap.end() ? nullptr : it->second;
}



