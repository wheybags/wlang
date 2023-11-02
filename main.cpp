#include "Tokeniser.hpp"
#include "Parser.hpp"
#include "PlainCGenerator.hpp"
#include "SemanticAnalyser.hpp"

std::string inputString = R"STRING_RAW(
  i32 ff(Vec2i* vec)
  {
    false;
    bool a = true;
    vec.x = 1;
    Vec2i vec2;
    func(1,2);
    return vec2.x;
  }

  class Vec2i
  {
    i32 x = 0 + 23;
    i32 y = 0;
  }

  // blah blah
  bool func(i32 x, i32 y)
  {
    Vec2i pos;
    i32 val = x + 2 * y / 3 + pos.x; // undefined = poop()

    if (1 == 1)
    {
      return 1 == 1;
    }
    else if (1 == 2)
    {
      return 1 == 2;
    }
    else
    {
      return 1 != 1;
    }

    return x == 1 && 1 == 1 || 1 == 1;
  }

  i32 main()
  {
    i32 var;
    var = 1;
    i32 var2 = 2;
    bool aa = 1 != 2;
    bool notAa = !aa;
    i32 negative = -var2 + 12;
    1;
    func(var, var2 + 3 - 1);
    return 0;
  }
)STRING_RAW";

int main()
{
  std::vector<Token> tokens = tokenise(inputString);
  Parser parser;
  Root* ast = parser.parse(tokens);

  std::string json;
  dumpJson(ast, json);
  puts(json.c_str());
  puts("\n");

  SemanticAnalyser semanticAnalyser(parser);
  semanticAnalyser.run(ast);

  PlainCGenerator generator;
  puts(generator.generate(ast).c_str());

  return 0;
}
