#pragma once
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include "../Assert.hpp"

struct NonTerminal;
struct ProductionItem
{
  explicit ProductionItem(std::string str) : _str(std::move(str)) {}
  explicit ProductionItem(NonTerminal& nonTerminal) : _nonTerminal(&nonTerminal) {}

  ProductionItem() = default;
  ProductionItem(const ProductionItem&) = default;
  ProductionItem(ProductionItem&&) noexcept = default;

  ProductionItem& operator=(const ProductionItem&) = default;
  ProductionItem& operator=(ProductionItem&&) noexcept = default;

  bool operator==(const ProductionItem& other) const = default;
  bool operator!=(const ProductionItem& other) const = default;

  bool operator==(std::string_view str) const { return isStr() && this->_str == str; }
  bool operator!=(std::string_view str) const { return !(*this == str); }

  bool operator==(const NonTerminal& nonTerminal) const { return isNonTerminal() && this->_nonTerminal == &nonTerminal; }
  bool operator!=(const NonTerminal& nonTerminal) const { return !(*this == nonTerminal); }

  bool isStr() const { return !_nonTerminal; }
  bool isNonTerminal() const { return _nonTerminal; }

  const std::string& str() const { release_assert(!_nonTerminal); return _str; }
  const NonTerminal& nonTerminal() const { release_assert(_nonTerminal); return *_nonTerminal; }

  void setNonTerminal(NonTerminal& nonTerminal)
  {
    this->_nonTerminal = &nonTerminal;
    this->_str.clear();
  }

  std::string codeInsertBefore;
  std::string codeInsertAfter;

  std::string parameters;

private:
  std::string _str;
  NonTerminal* _nonTerminal = nullptr;
};

using Production = std::vector<ProductionItem>;

std::string dumpProduction(const Production& production);

struct NonTerminal
{
  std::string name;
  std::string returnType;
  std::string arguments;
  std::string codeInsertAfter;
  std::vector<Production> productions;
};

class Grammar
{
public:
  explicit Grammar(const std::string& str_table);

  bool can_be_nil(const std::string& name) const;
  std::vector<std::vector<std::string>> first(const std::string& name) const;
  std::unordered_set<std::string> follow(const std::string& _name) const;

  const std::unordered_map<std::string, NonTerminal>& getRules() const { return rules; }
  const std::vector<std::string>& getKeys() const { return keys; }

private:
  std::unordered_map<std::string, NonTerminal> rules;
  std::vector<std::string> keys; // keys into rules map, in the order they were declared in the source
};