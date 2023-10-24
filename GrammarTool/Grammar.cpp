#include <functional>
#include <sstream>
#include "Grammar.hpp"


// Man, I forgot a lot of my compiler design theory since college...
// https://www.tutorialspoint.com/compiler_design/compiler_design_syntax_analysis.htm
// https://stackoverflow.com/questions/20317198/purpose-of-first-and-follow-sets-in-ll1-parsers

// https://stackoverflow.com/a/46931770
static std::vector<std::string> split(const std::string &s, char delim)
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

static std::vector<std::string> split(const std::string& s, const std::string& delimiter)
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

static BasicResult str_to_basic(const std::string& str_table)
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

struct GrammarResult
{
  std::unordered_map<std::string, NonTerminal> rules;
  std::vector<std::string> keys;
};

static GrammarResult make_grammar(const std::string& str_table)
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
          if (item == "Nil")
            release_assert(false && "Nil can only appear alone");
        }
      }

      if (production.size() == 1 &&
          production[0] == "Nil" &&
          prod_index != int32_t(rules_temp.rules[name].size()) - 1)
      {
        release_assert(false && "Nil has to be the last production");
      }

      for (int32_t i = 0; i < int32_t(production.size()); i++)
      {
        if (production[i] != "Nil" && isupper(production[i].str()[0]))
          production[i] = ProductionItem(rules.rules[production[i].str()]);
      }

      rules.rules[name].productions.emplace_back(std::move(production));
    }
  }

  return rules;
}


Grammar::Grammar(const std::string& str_table)
{
  GrammarResult result = make_grammar(str_table);
  this->rules = std::move(result.rules);
  this->keys = std::move(result.keys);
}

bool Grammar::can_be_nil(const std::string& name) const
{
  for (const Production& production : this->rules.at(name).productions)
  {
    if (production[0] == "Nil")
      return true;

    if (production[0].isNonTerminal())
    {
      Production test_prod = production;
      while (true)
      {
          if (!test_prod[0].isNonTerminal())
              break;
          if (!this->can_be_nil(test_prod[0].nonTerminal().name))
              break;

          test_prod.erase(test_prod.begin());

          if (test_prod.empty())
              return true;
      }
    }
  }

  return false;
}

std::vector<std::vector<std::string>> Grammar::first(const std::string& name) const
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
      if (!production[i].isNonTerminal())
      {
        prod_firsts_append(production[i].str());
        break;
      }

      const NonTerminal& nonTerminal = production[i].nonTerminal();

      for (const std::vector<std::string>& x : this->first(nonTerminal.name))
      {
        for (const std::string& item: x)
          prod_firsts_append(item);
      }

      if (!this->can_be_nil(nonTerminal.name))
        break;
    }

    if (!prod_firsts.empty())
      retval.emplace_back(std::move(prod_firsts));
  }

  // check for duplicates
  {
    std::unordered_set<std::string> tmp;
    for (const std::vector<std::string>& x: retval)
    {
      for (const std::string& item: x)
      {
        release_assert(!tmp.contains(item) && "duplicate first!");
        tmp.insert(item);
      }
    }
  }

  return retval;
}

std::unordered_set<std::string> Grammar::follow(const std::string& _name) const
{
  std::unordered_set<std::string> _visited;
  std::function<std::unordered_set<std::string>(const std::string&)> followInner = [&](const std::string& name) -> std::unordered_set<std::string>
  {
    if (_visited.contains(name))
      return {};

    _visited.insert(name);

    std::unordered_set<std::string> follows;
    const NonTerminal& nt = this->rules.at(name);

    for (const auto& pair : this->rules)
    {
      const NonTerminal& other_nt = pair.second;
      for (const Production& production: other_nt.productions)
      {
        for (int32_t i = 0; i < int32_t(production.size()); i++)
        {
          if (production[i] == nt)
          {
            Production prod_temp(production.begin() + i, production.end());
            while (true)
            {
              if (prod_temp.size() == 1)
              {
                if (prod_temp[0].isStr() || prod_temp[0] == nt || this->can_be_nil(prod_temp[0].nonTerminal().name))
                {
                  std::unordered_set<std::string> temp = followInner(other_nt.name);
                  follows.insert(temp.begin(), temp.end());
                }

                break;
              }
              else
              {
                auto& next_symbol = prod_temp[1];
                if (next_symbol.isNonTerminal())
                {
                  const NonTerminal& next_nt = next_symbol.nonTerminal();
                  for (const std::vector<std::string>& x : this->first(next_nt.name))
                  {
                    for (const std::string& item : x)
                      follows.insert(item);
                  }

                  if (!this->can_be_nil(next_nt.name))
                    break;
                }
                else
                {
                  follows.insert(next_symbol.str());
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

std::string dumpProduction(const Production& production)
{
  std::string retval;
  for (const auto& item : production)
  {
    if (item.isStr())
      retval += item.str();
    else
      retval += "NT(" + item.nonTerminal().name + ")";

    retval += " ";
  }

  if (!retval.empty())
    retval.resize(retval.size() - 1);

  return retval;
}