#include "Tokeniser.hpp"
#include "StringUtil.hpp"

std::vector<std::string> tokenise(const std::string& input)
{
  std::vector<std::string> tokens;
  std::string accumulator;
  accumulator.reserve(1024);

  auto breakToken = [&]()
  {
      if (!accumulator.empty())
        tokens.emplace_back(accumulator);
      accumulator.clear();
  };

  for (size_t i = 0; i < input.size(); i++)
  {
    char c = input[i];

    if (Str::isSpace(c))
    {
      breakToken();
    }
    else if (c == '(' || c == ')' || c == '{' || c== '}' || c == ';')
    {
      breakToken();
      tokens.emplace_back(std::string(1, c));
    }
    else if (c == '=')
    {
      breakToken();

      if (i+1 < input.size() && input[i+1] == '=')
      {
        tokens.emplace_back("==");
        i++;
      }
      else
      {
        tokens.emplace_back(std::string(1, c));
      }
    }
    else
    {
      accumulator += c;
    }
  }

  return tokens;
}