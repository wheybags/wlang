#include "Tokeniser.hpp"
#include "Parser.hpp"
#include "PlainCGenerator.hpp"

std::string inputString = R"STRING_RAW(
  class Vec2i
  {
    i32 x = 0;
    i32 y = 0;
  }

  i32 func(i32 x, i32 y)
  {
    Vec2i pos;
    i32 val = x + 2 * y / 3;
    return x == 1 && 1 || 1;
  }

  i32 main()
  {
    i32 var;
    var = 1;
    i32 var2 = 2;
    i32 aa = 1 != 2;
    i32 notAa = 2 + !aa + 1;
    i32 negativeAa = -aa + 12;
    1;
    func(var, var2 + 3 - 1);
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
