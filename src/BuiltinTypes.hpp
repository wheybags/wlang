#pragma once
#include "Ast.hpp"

struct BuiltinTypes
{
  Type tI32 = make("i32");
  Type tBool = make("bool");

  Type* get(std::string_view name);

  static BuiltinTypes inst;

public:
  HashMap<Type*> typeMap;

private:
  static Type make(std::string&& name) { return { .name = std::move(name), .builtin = true, }; }
  BuiltinTypes();
};

