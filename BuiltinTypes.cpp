#include "BuiltinTypes.hpp"

BuiltinTypes BuiltinTypes::inst;

Type* BuiltinTypes::get(std::string_view name)
{
  Type* it = (&tI32);
  while (it <= &tBool)
  {
    if (it->name == name)
      return it;
    it++;
  }
  return nullptr;
}

