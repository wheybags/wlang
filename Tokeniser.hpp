#pragma once
#include <vector>
#include <string>
#include <unordered_set>

struct Token
{
  enum class Type
  {
    Id,
    Int32,
    Comma,
    OpenBracket,
    CloseBracket,
    OpenBrace,
    CloseBrace,
    CompareEqual,
    LogicalAnd,
    Return,
    Semicolon,
    Assign,
    End
  };

  Type type = {};
  std::string idValue = {};
  int32_t i32Value = {};
};

using TT = Token::Type;

std::vector<Token> tokenise(std::string_view input);