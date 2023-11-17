#pragma once
#include "Ast.hpp"

struct BuiltinTypes
{
  Type tI32 { .name = "i32", .builtin = true };
  Type tBool { .name = "bool", .builtin = true };

  Type* get(std::string_view name);

  static BuiltinTypes inst;

private:
  BuiltinTypes() = default;
};

