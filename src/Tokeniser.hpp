#pragma once
#include <vector>
#include <string>
#include <unordered_set>

struct SourceLocation
{
  int32_t y = -1;
  int32_t x = -1;

  SourceLocation() = default;
  SourceLocation(int32_t x, int32_t y) : y(y), x(x) {}

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

struct IntegerToken
{
  int64_t val = 0;
  int32_t size = 0;
};

struct Token
{
  enum class Type
  {
    Id,
    IntegerToken,
    String,
    Comma,
    OpenBracket,
    CloseBracket,
    OpenBrace,
    CloseBrace,
    OpenSquareBracket,
    CloseSquareBracket,
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
    Extern,
    End
  };

  Type type = {};
  std::string idValue = {};
  IntegerToken integerValue;
  std::string stringValue;

  SourceRange source;
};

using TT = Token::Type;

std::vector<Token> tokenise(std::string_view input);