#include "Tokeniser.hpp"
#include "Parser.hpp"
#include "PlainCGenerator.hpp"

std::string inputString = R"STRING_RAW(

i32 func(i32 x)
{
  return x == 1;
}

i32 main()
{
    return 0;
}

)STRING_RAW";

int main()
{
  std::vector<std::string> tokens = tokenise(inputString);
  ParseTree parser;
  const Root* ast = parser.parse(tokens);

//  std::string json;
//  dumpJson(ast, json);
//  puts(json.c_str());

  PlainCGenerator generator;
  puts(generator.generate(ast).c_str());

  return 0;
}
