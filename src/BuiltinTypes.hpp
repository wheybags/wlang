#pragma once
#include "Ast.hpp"

struct BuiltinTypes
{
  Type tI8 = makeNumeric("i8");
  Type tI16 = makeNumeric("i16");
  Type tI32 = makeNumeric("i32");
  Type tI64 = makeNumeric("i64");
  Type tBool = make("bool");
  Type tNull = make("nullT");

  Type* get(std::string_view name);

  static BuiltinTypes inst;

public:
  HashMap<Type*> typeMap;

public:
  static Type* resolveBinaryOperatorPromotion(Type* left, Type* right);

private:
  static Type make(std::string&& name) { return { .name = std::move(name), .builtin = true, }; }
  static Type makeNumeric(std::string&& name) { return { .name = std::move(name), .builtin = true, .builtinNumeric = true }; }
  BuiltinTypes();
};

