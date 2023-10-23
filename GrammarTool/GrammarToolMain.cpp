#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <sstream>
#include <variant>
#include <functional>
#include <algorithm>
#include "../Assert.hpp"


// Man, I forgot a lot of my compiler design theory since college...
// https://www.tutorialspoint.com/compiler_design/compiler_design_syntax_analysis.htm
// https://stackoverflow.com/questions/20317198/purpose-of-first-and-follow-sets-in-ll1-parsers

const char* grammar = R"STR(
  Root                = FuncList $End
  FuncList            = Func FuncList'
  FuncList'           = FuncList | Nil
  Func                = Type $Id "(" Func'
  Func'               = ArgList ")" Block | ")" Block
  Block               = "{" StatementList "}"
  StatementList       = Statement ";" StatementList'
  StatementList'      = StatementList | Nil
  Statement           = "return" Expression | $Id Statement'

  //                    Decl  Assign
  Statement'          = $Id | Expression' "=" Expression
  Expression          = $Id Expression' | $Int32 Expression'
  Expression'         = "==" Expression | "&&" Expression | Nil
  Type                = $Id
  ArgList             = Arg ArgList'
  ArgList'            = "," ArgList | Nil
  Arg                 = Type $Id
)STR";

// https://stackoverflow.com/a/46931770
std::vector<std::string> split(const std::string &s, char delim)
{
  std::vector<std::string> result;
  std::stringstream ss(s);
  std::string item;

  while (getline(ss, item, delim))
  {
    if (!item.empty())
      result.push_back(item);
  }

  return result;
}

std::vector<std::string> split(const std::string& s, const std::string& delimiter)
{
  size_t pos_start = 0, pos_end, delim_len = delimiter.length();
  std::string token;
  std::vector<std::string> res;

  while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
  {
    token = s.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + delim_len;

    if (!token.empty())
      res.push_back(token);
  }

  res.push_back(s.substr(pos_start));
  return res;
}

struct BasicResult
{
  std::unordered_map<std::string, std::vector<std::vector<std::string>>> rules;
  std::vector<std::string> keys;
};

BasicResult str_to_basic(const std::string& str_table)
{
  BasicResult rules_temp;

  for (const std::string& lineStr : split(str_table, '\n'))
  {
    std::vector<std::string> line = split(lineStr, "//");
    line = split(line[0], ' ');

    if (line.empty())
        continue;

    std::vector<std::vector<std::string>> productions;
    std::vector<std::string> acc;

    release_assert(line.size() > 2);
    release_assert(line[1] == "=");
    for (int32_t i = 2; i < int32_t(line.size()); i++)
    {
      if (line[i] == "|")
      {
        productions.emplace_back(std::move(acc));
        acc.clear();
      }
      else
      {
        acc.emplace_back(std::move(line[i]));
      }
    }

    if (!acc.empty())
      productions.emplace_back(std::move(acc));

    rules_temp.rules[line[0]] = productions;
    rules_temp.keys.emplace_back(line[0]);
  }

  return rules_temp;
}

struct NonTerminal;
using Production = std::vector<std::variant<std::string, NonTerminal*>>;

template <typename T, typename... VariantTypes>
bool variantEquals(const std::variant<VariantTypes...>& a, const T& b)
{
  return std::holds_alternative<T>(a) && std::get<T>(a) == b;
}

template <typename T, typename... VariantTypes>
bool variantEquals(const T& a, const std::variant<VariantTypes...>& b)
{
  return variantEquals(b, a);
}

struct NonTerminal
{
  std::string name;
  std::vector<Production> productions;
};

std::string dumpProduction(const Production& production)
{
  std::string retval;
  for (const auto& item : production)
  {
    if (std::holds_alternative<std::string>(item))
      retval += std::get<std::string>(item);
    else
      retval += "NT(" + std::get<NonTerminal*>(item)->name + ")";

    retval += " ";
  }

  if (!retval.empty())
    retval.resize(retval.size() - 1);

  return retval;
}

struct GrammarResult
{
  std::unordered_map<std::string, NonTerminal> rules;
  std::vector<std::string> keys;
};

GrammarResult make_grammar(const std::string& str_table)
{
  BasicResult rules_temp = str_to_basic(str_table);

  GrammarResult rules;
  rules.keys = rules_temp.keys;

  for (const auto& pair: rules_temp.rules)
    rules.rules[pair.first] = NonTerminal { .name = pair.first };


  for (const auto& pair: rules.rules)
  {
    const std::string& name = pair.first;
    for (int32_t prod_index = 0; prod_index < int32_t(rules_temp.rules[name].size()); prod_index++)
    {
      Production production;
      for (const std::string& item : rules_temp.rules[name][prod_index])
        production.emplace_back(item);

      if (production.size() > 1)
      {
        for (const auto& item: production)
        {
          if (std::get<std::string>(item) == "Nil")
            release_assert(false && "Nil can only appear alone");
        }
      }

      if (production.size() == 1 &&
          std::get<std::string>(production[0]) == "Nil" &&
          prod_index != int32_t(rules_temp.rules[name].size()) - 1)
      {
        release_assert(false && "Nil has to be the last production");
      }

      for (int32_t i = 0; i < int32_t(production.size()); i++)
      {
        if (std::get<std::string>(production[i]) != "Nil" && isupper(std::get<std::string>(production[i])[0]))
          production[i] = &rules.rules[std::get<std::string>(production[i])];
      }

      rules.rules[name].productions.emplace_back(std::move(production));
    }
  }

  return rules;
}


class Grammar
{
public:
  explicit Grammar(const std::string& str_table)
  {
    GrammarResult result = make_grammar(str_table);
    this->rules = std::move(result.rules);
    this->keys = std::move(result.keys);
  }

  bool can_be_nil(const std::string& name)
  {
    for (const Production& production : this->rules.at(name).productions)
    {
      if (variantEquals(production[0], std::string("Nil")))
        return true;

      if (std::holds_alternative<NonTerminal*>(production[0]))
      {
        Production test_prod = production;
        while (true)
        {
            if (!std::holds_alternative<NonTerminal*>(test_prod[0]))
                break;
            if (!this->can_be_nil(std::get<NonTerminal*>(test_prod[0])->name))
                break;

            test_prod.erase(test_prod.begin());

            if (test_prod.empty())
                return true;
        }
      }
    }

    return false;
  }

  std::vector<std::vector<std::string>> _first_internal(const std::string& name)
  {
    std::vector<std::vector<std::string>> retval;
    for (const Production& production : this->rules.at(name).productions)
    {
      std::vector<std::string> prod_firsts;
      auto prod_firsts_append = [&](const std::string& sym)
      {
        if (sym == "Nil" && std::find(prod_firsts.begin(), prod_firsts.end(), "Nil") != prod_firsts.end())
          return;
        prod_firsts.emplace_back(sym);
      };

      for (int32_t i = 0; i < int32_t(production.size()); i++)
      {
        if (!std::holds_alternative<NonTerminal*>(production[i]))
        {
          prod_firsts_append(std::get<std::string>(production[i]));
          break;
        }

        NonTerminal* nonTerminal = std::get<NonTerminal*>(production[i]);

        for (const std::vector<std::string>& x : this->first(nonTerminal->name))
        {
          for (const std::string& item: x)
            prod_firsts_append(item);
        }

        if (!this->can_be_nil(nonTerminal->name))
          break;
      }

      if (!prod_firsts.empty())
        retval.emplace_back(std::move(prod_firsts));
    }

    return retval;
  }

  std::vector<std::vector<std::string>> first(const std::string& name)
  {
    std::vector<std::vector<std::string>> retval = this->_first_internal(name);

    std::unordered_set<std::string> tmp;
    for (const std::vector<std::string>& x : retval)
    {
      for (const std::string& item : x)
      {
        release_assert(!tmp.contains(item) && "duplicate first!");
        tmp.insert(item);
      }
    }

    return retval;
  }

  std::string _pad(const std::string& name)
  {
    int32_t max_len = 0;
    for (const auto& pair: this->rules)
      max_len = std::max(int32_t(pair.first.size()), max_len);

    std::string retval;
    for (int32_t i = 0; i < (max_len - int32_t(name.size())); i++)
      retval += ' ';

    return retval;
  }

  std::string str_first(const std::string& name)
  {
    std::string retval = name + " " + this->_pad(name) + "= ";

    for (const std::vector<std::string>& prod_firsts : this->first(name))
    {
      for (const std::string& item : prod_firsts)
        retval += item + " ";
      retval += "| ";
    }

    if (!retval.empty())
      retval.resize(retval.size() - 3);

    return retval;
  }

  std::unordered_set<std::string> follow(const std::string& _name)
  {
    std::unordered_set<std::string> _visited;
    std::function<std::unordered_set<std::string>(const std::string&)> followInner = [&](const std::string& name) -> std::unordered_set<std::string>
    {
      if (_visited.contains(name))
        return {};

      _visited.insert(name);

      std::unordered_set<std::string> follows;
      NonTerminal* nt = &this->rules[name];

      for (auto& pair : this->rules)
      {
        NonTerminal* other_nt = &pair.second;
        for (const Production& production: other_nt->productions)
        {
          for (int32_t i = 0; i < int32_t(production.size()); i++)
          {
            if (variantEquals(nt, production[i]))
            {
              Production prod_temp(production.begin() + i, production.end());
              while (true)
              {
                if (prod_temp.size() == 1)
                {
                  if (std::holds_alternative<std::string>(prod_temp[0]) ||
                      variantEquals(prod_temp[0], nt) ||
                      this->can_be_nil(std::get<NonTerminal*>(prod_temp[0])->name))
                  {
                    std::unordered_set<std::string> temp = followInner(other_nt->name);
                    follows.insert(temp.begin(), temp.end());
                  }

                  break;
                }
                else
                {
                  auto& next_symbol = prod_temp[1];
                  if (std::holds_alternative<NonTerminal*>(next_symbol))
                  {
                    NonTerminal* next_nt = std::get<NonTerminal*>(next_symbol);
                    for (const std::vector<std::string>& x : this->first(next_nt->name))
                    {
                      for (const std::string& item : x)
                        follows.insert(item);
                    }

                    if (!this->can_be_nil(next_nt->name))
                      break;
                  }
                  else
                  {
                    follows.insert(std::get<std::string>(next_symbol));
                    break;
                  }
                }

                prod_temp.erase(prod_temp.begin());
              }
            }
          }
        }
      }

      follows.erase("Nil");

      return follows;
    };

    return followInner(_name);
  }

  std::string str_follow(const std::string& name)
  {
    std::string retval = name + " " + this->_pad(name) + "= ";

    std::unordered_set<std::string> follows = this->follow(name);
    std::vector<std::string> sortedFollows(follows.begin(), follows.end());
    std::sort(sortedFollows.begin(), sortedFollows.end());

    for (const std::string& item : sortedFollows)
      retval += item + " ";

    if (!retval.empty())
      retval.resize(retval.size() - 1);

    return retval;
  }

public:
  std::unordered_map<std::string, NonTerminal> rules;
  std::vector<std::string> keys;
};


void test_can_be_nil()
{
  Grammar rules(R"STR(
    Root = "3" Y $End
    Y = A | "1" // comment
    A = "2" | Nil
  )STR");

  release_assert(!rules.can_be_nil("Root"));
  release_assert(rules.can_be_nil("A"));
  release_assert(rules.can_be_nil("Y"));


  rules = Grammar(R"STR(
    Root = "x" Y $End
    Y = A B C D
    A = "1" | Nil
    B = "2" | Nil
    C = "3" | Nil
    D = "4" | Nil
  )STR");

  release_assert(!rules.can_be_nil("Root"));
  release_assert(rules.can_be_nil("A"));
  release_assert(rules.can_be_nil("B"));
  release_assert(rules.can_be_nil("C"));
  release_assert(rules.can_be_nil("D"));
  release_assert(rules.can_be_nil("Y"));


  rules = Grammar(R"STR(
      Root = "x" Y
      Y = A B C D
      A = "1" | Nil
      B = "2" | Nil
      C = "3"
      D = "4" | Nil
  )STR");

  release_assert(!rules.can_be_nil("Root"));
  release_assert(rules.can_be_nil("A"));
  release_assert(rules.can_be_nil("B"));
  release_assert(!rules.can_be_nil("C"));
  release_assert(rules.can_be_nil("D"));
  release_assert(!rules.can_be_nil("Y"));
}

using vvs = std::vector<std::vector<std::string>>;

void test_first()
{
  Grammar rules(R"STR(
        Root = "3" Y $End
        Y = A | "1"
        A = "2" | Nil
  )STR");
  release_assert(rules.first("Root") == vvs{{"\"3\""}});
  release_assert((rules.first("A") == vvs{{"\"2\""}, {"Nil"}}));
  release_assert((rules.first("Y") == vvs{{"\"2\"", "Nil"}, {"\"1\""}}));

  rules = Grammar(R"STR(
      Root = "3" Y $End
      Y = A | "1"
      A = "2" | "4"
  )STR");
  release_assert(rules.first("Root") == vvs{{"\"3\""}});
  release_assert((rules.first("A") == vvs{{"\"2\""}, {"\"4\""}}));
  release_assert((rules.first("Y") == vvs{{"\"2\"", "\"4\""}, {"\"1\""}}));

  rules = Grammar(R"STR(
      Root = "x" Y $End
      Y = A B C D
      A = "1" | Nil
      B = "2" | Nil
      C = "3"
      D = "4" | Nil
  )STR");
  release_assert((rules.first("A") == vvs{{"\"1\""}, {"Nil"}}));
  release_assert((rules.first("B") == vvs{{"\"2\""}, {"Nil"}}));
  release_assert(rules.first("C") == vvs{{"\"3\""}});
  release_assert((rules.first("D") == vvs{{"\"4\""}, {"Nil"}}));
  release_assert((rules.first("Y") == vvs{{"\"1\"", "Nil", "\"2\"", "\"3\""}}));

  rules = Grammar(R"STR(
      Root = "x" Y $End
      Y = A B C D
      A = "1" | Nil
      B = "2" | Nil
      C = "3" | Nil
      D = "4" | Nil
  )STR");
  release_assert((rules.first("A") == vvs{{"\"1\""}, {"Nil"}}));
  release_assert((rules.first("B") == vvs{{"\"2\""}, {"Nil"}}));
  release_assert((rules.first("C") == vvs{{"\"3\""}, {"Nil"}}));
  release_assert((rules.first("D") == vvs{{"\"4\""}, {"Nil"}}));
  release_assert((rules.first("Y") == vvs{{"\"1\"", "Nil", "\"2\"", "\"3\"", "\"4\""}}));
}

using ss = std::unordered_set<std::string>;

void test_follow()
{
  Grammar rules(R"STR(
      Root = A Y $End
      Y = A | "1"
      A = "2" | "3"
  )STR");

  release_assert((rules.follow("Root") == ss{}));
  release_assert((rules.follow("A") == ss{"\"1\"", "\"2\"", "\"3\"", "$End"}));
  release_assert((rules.follow("Y") == ss{"$End"}));

  rules = Grammar(R"STR(
      Root = "x" Y $End
      Y = A B C D
      A = "1" | Nil
      B = "2" | Nil
      C = "3"
      D = "4" | Nil
  )STR");

  release_assert((rules.follow("Root") == ss{}));
  release_assert((rules.follow("Y") == ss{"$End"}));
  release_assert((rules.follow("A") == ss{"\"2\"", "\"3\""}));
  release_assert((rules.follow("B") == ss{"\"3\""}));
  release_assert((rules.follow("C") == ss{"\"4\"", "$End"}));
}

void test()
{
  test_can_be_nil();
  test_first();
  test_follow();
}

int main()
{
  test();

  Grammar rules(grammar);

  printf("Firsts:\n");
  for (const std::string& name : rules.keys)
    printf("    %s\n", rules.str_first(name).c_str());

  printf("\nFollows:\n");
  for (const std::string& name : rules.keys)
    printf("    %s\n", rules.str_follow(name).c_str());

  return 0;
}