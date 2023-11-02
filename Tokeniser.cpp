#include "Tokeniser.hpp"
#include "StringUtil.hpp"
#include "Assert.hpp"
#include <map>
#include <optional>

const std::vector<std::pair<std::string_view, Token::Type>> keywords
{
  {"return", Token::Type::Return},
  {"class", Token::Type::Class},
  {"false", Token::Type::False},
  {"else", Token::Type::Else},
  {"true", Token::Type::True},
  {"if", Token::Type::If},
  {"==", Token::Type::CompareEqual},
  {"!=", Token::Type::CompareNotEqual},
  {"&&", Token::Type::LogicalAnd},
  {"||", Token::Type::LogicalOr},
  {"!", Token::Type::LogicalNot},
  {",", Token::Type::Comma},
  {"(", Token::Type::OpenBracket},
  {")", Token::Type::CloseBracket},
  {"{", Token::Type::OpenBrace},
  {"}", Token::Type::CloseBrace},
  {";", Token::Type::Semicolon},
  {"=", Token::Type::Assign},
  {"+", Token::Type::Add},
  {"-", Token::Type::Subtract},
  {"*", Token::Type::Asterisk},
  {"/", Token::Type::Divide},
  {".", Token::Type::Dot},
};

std::optional<int32_t> parseInt32(std::string_view str)
{
  // TODO: overflow check

  int32_t value = 0;

  for (char c : str)
  {
    if (Str::isNumeric(c))
      value = value * 10 + (int32_t(c) - int32_t('0'));
    else
      return std::nullopt;
  }

  return value;
}

std::vector<Token> tokenise(std::string_view input)
{
  std::vector<Token> tokens;

  Token::Type accumulatorType;
  std::string accumulator;
  accumulator.reserve(1024);

  auto breakToken = [&]()
  {
      if (accumulator.empty())
        return;

      if (accumulatorType == Token::Type::Id)
        tokens.emplace_back(Token{accumulatorType, accumulator});
      else if (accumulatorType == Token::Type::Int32)
        tokens.emplace_back(Token{accumulatorType, std::string(), *parseInt32(accumulator)});

      accumulator.clear();
  };

  auto advance = [&](size_t chars)
  {
    debug_assert(input.size() >= chars);
    input = std::string_view(input.data() + chars, input.size() - chars);
  };

  while (!input.empty())
  {
    if (Str::isSpace(input[0]))
    {
      breakToken();
      advance(1);
      continue;
    }

    if (accumulator.empty())
    {
      bool found = false;
      for (const auto& pair : keywords)
      {
        if (Str::startsWith(input, pair.first))
        {
          tokens.push_back(Token{pair.second});
          advance(pair.first.size());
          found = true;
          break;
        }
      }

      if (found)
        continue;

      if (input[0] == '_' || Str::isAlpha(input[0]))
      {
        accumulator.push_back(input[0]);
        accumulatorType = Token::Type::Id;
        advance(1);
        continue;
      }
      else if (Str::isAlphaNumeric(input[0]))
      {
        accumulator.push_back(input[0]);
        accumulatorType = Token::Type::Int32;
        advance(1);
        continue;
      }
      else
      {
        message_and_abort("invalid token");
      }
    }

    if (accumulatorType == Token::Type::Int32 && !Str::isNumeric(input[0]))
    {
      breakToken();
      continue;
    }
    if (accumulatorType == Token::Type::Id && !(Str::isAlphaNumeric(input[0]) || input[0] == '_'))
    {
      breakToken();
      continue;
    }

    accumulator.push_back(input[0]);
    advance(1);
  }

  breakToken();

  tokens.push_back(Token{Token::Type::End});
  return tokens;
}