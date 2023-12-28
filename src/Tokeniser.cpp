#include "Tokeniser.hpp"
#include "Common/StringUtil.hpp"
#include "Common/Assert.hpp"
#include <optional>

const std::vector<std::pair<std::string_view, Token::Type>> keywordMapping
{
  {"return", Token::Type::Return},
  {"extern", Token::Type::Extern},
  {"class",  Token::Type::Class},
  {"false",  Token::Type::False},
  {"else",   Token::Type::Else},
  {"true",   Token::Type::True},
  {"null",   Token::Type::Null},
  {"if",     Token::Type::If},
};

const std::vector<std::pair<std::string_view, Token::Type>> tokenMapping
{
  {"==", Token::Type::CompareEqual},
  {"!=", Token::Type::CompareNotEqual},
  {"&&", Token::Type::LogicalAnd},
  {"||", Token::Type::LogicalOr},
  {"!", Token::Type::LogicalNot},
  {",", Token::Type::Comma},
  {"(", Token::Type::OpenBracket},
  {")", Token::Type::CloseBracket},
  {"[", Token::Type::OpenSquareBracket},
  {"]", Token::Type::CloseSquareBracket},
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

std::optional<int64_t> parseInteger(std::string_view str)
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

  enum class Type
  {
    Id,
    Integer,
    Comment,
    String,
    None,
  };

  Type accumulatorType = Type::None;
  int32_t integerSize = -1;
  bool escaped = false;
  int32_t accumulatorStartY = -1;
  int32_t accumulatorStartX = -1;
  std::string accumulator;
  accumulator.reserve(1024);

  int32_t accumulatorY = 1;
  int32_t accumulatorX = 1;
  bool wasNewline = false;

  auto breakToken = [&]()
  {
    if (accumulator.empty())
      return;

    SourceRange source({accumulatorStartX, accumulatorStartY}, {accumulatorX, accumulatorY});

    if (accumulatorType == Type::Id)
      tokens.emplace_back(Token{.type = Token::Type::Id, .idValue = accumulator, .source = source});
    else if (accumulatorType == Type::Integer)
      tokens.emplace_back(Token{.type = Token::Type::IntegerToken, .integerValue = {.val = *parseInteger(accumulator), .size = integerSize}, .source = source});
    else if (accumulatorType == Type::String)
      tokens.emplace_back(Token{.type = Token::Type::String, .stringValue = accumulator, .source = source});

    accumulator.clear();
    accumulatorType = Type::None;
  };

  auto accumulate = [&](char c)
  {
    if (accumulator.empty())
    {
      accumulatorStartY = accumulatorY;
      accumulatorStartX = accumulatorX;
    }
    accumulator += c;
  };

  auto advance = [&](size_t chars)
  {
    debug_assert(input.size() >= chars);
    input = std::string_view(input.data() + chars, input.size() - chars);
  };

  while (!input.empty())
  {
    accumulatorX++;

    if (wasNewline)
    {
      accumulatorY++;
      accumulatorX = 1;
      wasNewline = false;
    }

    if (input[0] == '\n')
      wasNewline = true;

    if (accumulatorType == Type::Comment)
    {
      if (input[0] == '\n')
        accumulatorType = Type::None;

      advance(1);
      continue;
    }

    if (accumulatorType == Type::String)
    {
      if (escaped)
      {
        escaped = false;
      }
      else
      {
        if (input[0] == '"')
        {
          accumulator += '"';
          advance(1);
          breakToken();
          continue;
        }

        if (input[0] == '\\')
          escaped = true;
      }

      accumulator += input[0];
      advance(1);
      continue;
    }

    if (Str::isSpace(input[0]))
    {
      breakToken();
      advance(1);
      continue;
    }

    if (input.starts_with("//"))
    {
      breakToken();
      accumulatorType = Type::Comment;
      advance(2);
      continue;
    }

    if (accumulator.empty())
    {
      bool found = false;

      for (const auto& pair : tokenMapping)
      {
        std::string_view keyword = pair.first;
        if (Str::startsWith(input, keyword))
        {
          SourceRange source({accumulatorX, accumulatorY}, {accumulatorX + int32_t(keyword.size()), accumulatorY});
          tokens.push_back(Token{.type = pair.second, .source = source});
          advance(keyword.size());
          found = true;
          break;
        }
      }

      for (const auto& pair : keywordMapping)
      {
        std::string_view keyword = pair.first;
        if (Str::startsWith(input, keyword))
        {
          bool keywordEnds = (input.size() >= keyword.size() + 1 && !Str::isAlpha(input[keyword.size()])) || input.size() == keyword.size();
          if (keywordEnds)
          {
            SourceRange source({accumulatorX, accumulatorY}, {accumulatorX + int32_t(keyword.size()), accumulatorY});
            tokens.push_back(Token{.type = pair.second, .source = source});
            advance(pair.first.size());
            found = true;
            break;
          }
        }
      }

      if (found)
        continue;

      if (input[0] == '_' || Str::isAlpha(input[0]))
      {
        accumulate(input[0]);
        accumulatorType = Type::Id;
        advance(1);
        continue;
      }
      else if (Str::isAlphaNumeric(input[0]))
      {
        accumulate(input[0]);
        accumulatorType = Type::Integer;
        integerSize = 32; // default
        advance(1);
        continue;
      }
      else if (input[0] == '"')
      {
        accumulate(input[0]);
        accumulatorType = Type::String;
        escaped = false;
        advance(1);
      }
      else
      {
        message_and_abort("invalid token");
      }
    }

    if (accumulatorType == Type::Integer && !Str::isNumeric(input[0]))
    {
      if (input.starts_with("i64"))
      {
        integerSize = 64;
        advance(3);
      }
      else if (input.starts_with("i32"))
      {
        integerSize = 32;
        advance(3);
      }
      else if (input.starts_with("i16"))
      {
        integerSize = 16;
        advance(3);
      }
      else if (input.starts_with("i8"))
      {
        integerSize = 8;
        advance(2);
      }

      breakToken();
      continue;
    }
    if (accumulatorType == Type::Id && !(Str::isAlphaNumeric(input[0]) || input[0] == '_'))
    {
      breakToken();
      continue;
    }

    accumulate(input[0]);
    advance(1);
  }

  breakToken();


  SourceRange source({accumulatorX, accumulatorY}, {accumulatorX, accumulatorY});
  tokens.push_back(Token{.type = Token::Type::End, .source = source});
  return tokens;
}