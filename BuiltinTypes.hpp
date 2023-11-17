#pragma once
#include "Ast.hpp"

struct BuiltinTypes
{
  Type tI32 { .name = "i32" };
  Type tBool { .name = "bool" };

  Type* get(std::string_view name);

  static BuiltinTypes inst;

private:
  BuiltinTypes() = default;
};

