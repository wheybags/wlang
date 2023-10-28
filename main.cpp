#include "Tokeniser.hpp"
#include "Parser.hpp"
#include "PlainCGenerator.hpp"

std::string inputString = R"STRING_RAW(
  i32 f()
  {
    return 1;
  }

  i32 func(i32 x, i32 y)
  {
    return x == 1 && 1;
  }

  i32 main()
  {
    i32 var;
    var = 1;
    1;
    f();
    return 0;
  }
)STRING_RAW";

int main()
{
  std::vector<Token> tokens = tokenise(inputString);
  Parser parser;
  const Root* ast = parser.parse(tokens);

  std::string json;
  dumpJson(ast, json);
  puts(json.c_str());
  puts("\n");

  PlainCGenerator generator;
  puts(generator.generate(ast).c_str());

  return 0;
}
