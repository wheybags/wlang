#pragma once
#include <string>

class Grammar;

struct ParserSource
{
  std::string implementationSource;
  std::string declarationSource;
};
ParserSource generateParser(const Grammar& grammar);