#include "BuiltinTypes.hpp"

BuiltinTypes BuiltinTypes::inst;

BuiltinTypes::BuiltinTypes()
{
  Type* it = (&tI8);
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

Type* BuiltinTypes::resolveBinaryOperatorPromotion(Type* left, Type* right)
{
  // We differ from the C spec here, because it's... a bit crazy tbh.
  // C spec, for reference: https://stackoverflow.com/a/46073296

  auto getBits = [](Type* type) -> int32_t
  {
    if (type == &inst.tI8)
      return 8;
    if (type == &inst.tI16)
      return 16;
    if (type == &inst.tI32)
      return 32;
    if (type == &inst.tI64)
      return 64;
    message_and_abort("bad type");
  };

  if (left == right)
    return left;

  if (getBits(left) > getBits(right))
    return left;
  else
    return right;
}



