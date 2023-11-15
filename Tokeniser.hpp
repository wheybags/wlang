#pragma once
#include <vector>
#include <string>
#include <unordered_set>

struct SourceLocation
{
  int32_t y = -1;
  int32_t x = -1;

  SourceLocation() = default;
  SourceLocation(int32_t x, int32_t y) : x(x), y(y) {}

  bool operator>(SourceLocation other) const { return std::make_pair(y, x) > std::make_pair(other.y, other.x); }
  bool operator>=(SourceLocation other) const { return std::make_pair(y, x) >= std::make_pair(other.y, other.x); }
  bool operator<(SourceLocation other) const { return std::make_pair(y, x) < std::make_pair(other.y, other.x); }
  bool operator<=(SourceLocation other) const { return std::make_pair(y, x) <= std::make_pair(other.y, other.x); }
};

struct SourceRange
{
  SourceRange() = default;
  SourceRange(SourceLocation start, SourceLocation end) : start(start), end(end) {}

  SourceLocation start;
  SourceLocation end;

  bool isAfterEndOf(SourceRange other) const { return this->start >= other.end; }
};

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
    CompareNotEqual,
    LogicalAnd,
    LogicalOr,
    LogicalNot,
    Return,
    Semicolon,
    Assign,
    Add,
    Subtract,
    Asterisk,
    Divide,
    Class,
    Dot,
    If,
    Else,
    True,
    False,
    End
  };

  Type type = {};
  std::string idValue = {};
  int32_t i32Value = {};

  SourceRange source;
};

using TT = Token::Type;

std::vector<Token> tokenise(std::string_view input);