#include <functional>
#include <sstream>
#include "Grammar.hpp"
#include "../StringUtil.hpp"


// Man, I forgot a lot of my compiler design theory since college...
// https://www.tutorialspoint.com/compiler_design/compiler_design_syntax_analysis.htm
// https://stackoverflow.com/questions/20317198/purpose-of-first-and-follow-sets-in-ll1-parsers

static std::vector<std::string> tokenise(std::string_view input)
{
  std::vector<std::string> tokens;

  bool inQuotedString = false;
  int32_t angleBracketDepth = 0;
  bool inComment = false;
  bool inCodeInsert = false;
  std::string accumulator;

  auto breakToken = [&]()
  {
    if (accumulator.empty())
      return;

    tokens.emplace_back(std::move(accumulator));
    accumulator.clear();
  };

  auto advance = [&](size_t chars)
  {
    debug_assert(input.size() >= chars);
    input = std::string_view(input.data() + chars, input.size() - chars);
  };

  while (!input.empty())
  {
    if (inComment)
    {
      if (input[0] == '\n')
        inComment = false;
      advance(1);
      continue;
    }

    if (!inQuotedString && !inComment && angleBracketDepth == 0 && !inCodeInsert)
    {
      if (Str::isSpace(input[0]))
      {
        breakToken();
        advance(1);
        continue;
      }

      if (input[0] == '|' || input[0] == '=' || input[0] == ';')
      {
        breakToken();
        accumulator.push_back(input[0]);
        breakToken();
        advance(1);
        continue;
      }

      if (input.starts_with("//"))
      {
        advance(2);
        inComment = true;
        continue;
      }

      if (input[0] == '<')
      {
        breakToken();
        advance(1);
        angleBracketDepth = 1;
        accumulator.push_back('<');
        continue;
      }

      if (input.starts_with("{{"))
      {
        breakToken();
        advance(2);
        inCodeInsert = true;
        accumulator += "{{";
        continue;
      }
    }

    if (accumulator.empty() && input[0] == '"')
    {
      advance(1);
      inQuotedString = true;
      accumulator.push_back('"');
      continue;
    }

    if (inQuotedString && input[0] == '"')
    {
      advance(1);
      inQuotedString = false;
      accumulator.push_back('"');
      breakToken();
      continue;
    }

    if (angleBracketDepth)
    {
      if (input[0] == '>')
      {
        angleBracketDepth--;

        if (angleBracketDepth == 0)
        {
          advance(1);
          accumulator.push_back('>');
          breakToken();
          continue;
        }
      }
      else if (input[0] == '<')
      {
        angleBracketDepth++;
      }
    }

    if (inCodeInsert && input.starts_with("}}"))
    {
      advance(2);
      inCodeInsert = false;
      accumulator += "}}";
      breakToken();
      continue;
    }

    accumulator.push_back(input[0]);
    advance(1);
  }

  return tokens;
}


struct GrammarResult
{
  std::unordered_map<std::string, NonTerminal> rules;
  std::vector<std::string> keys;
};

static GrammarResult make_grammar(const std::string& str_table)
{
  GrammarResult rules;
  {
    std::vector<std::string> tokens = tokenise(str_table);

    int32_t ruleStartIndex = 0;
    std::string currentRuleName;
    Production accumulator;

    std::string codeInsert;

    auto breakAccumulator = [&]()
    {
      if (!accumulator.empty())
      {
        if (!codeInsert.empty())
        {
          accumulator[accumulator.size()-1].codeInsertAfter = std::move(codeInsert);
          codeInsert.clear();
        }
        rules.rules[currentRuleName].productions.emplace_back(std::move(accumulator));
      }
      accumulator.clear();
    };

    for (int32_t i = 0; i < int32_t(tokens.size()); i++)
    {
      if (i == ruleStartIndex)
      {
        release_assert(tokens[i] != ";");
        release_assert((i < int32_t(tokens.size()) - 1 && tokens[i+1] == "=") ||
                       (i < int32_t(tokens.size()) - 2 && tokens[i+1][0] == '<' && tokens[i+2] == "="));
        rules.rules[tokens[i]] = NonTerminal { .name = tokens[i] };
        rules.keys.emplace_back(tokens[i]);
        currentRuleName = tokens[i];
        ruleStartIndex = i;
        continue;
      }

      if (tokens[i] == "=")
      {
        release_assert((i == ruleStartIndex + 1) ||
                       (i == ruleStartIndex + 2 && tokens[i-1][0] == '<'));
        continue;
      }

      if (tokens[i] == ";")
      {
        breakAccumulator();
        ruleStartIndex = i + 1;
        continue;
      }

      if (tokens[i] == "|")
      {
        breakAccumulator();
        continue;
      }

      if (tokens[i].starts_with("<"))
      {
        release_assert(tokens[i][tokens[i].length()-1] == '>');
        release_assert(i == ruleStartIndex + 1);
        rules.rules[currentRuleName].returnType = tokens[i].substr(1, tokens[i].length()-2);
        continue;
      }

      if (tokens[i].starts_with("{{"))
      {
        release_assert(tokens[i].ends_with("}}"));
        codeInsert = tokens[i].substr(2, tokens[i].length()-4);
        continue;
      }

      ProductionItem& item = accumulator.emplace_back(tokens[i]);
      if (!codeInsert.empty())
      {
        item.codeInsertBefore = std::move(codeInsert);
        codeInsert.clear();
      }
    }
  }

  for (const std::string& name: rules.keys)
  {
    for (int32_t prod_index = 0; prod_index < int32_t(rules.rules[name].productions.size()); prod_index++)
    {
      Production& production = rules.rules[name].productions[prod_index];

      if (production.size() > 1)
      {
        for (const auto& item: production)
        {
          if (item == "Nil")
            release_assert(false && "Nil can only appear alone");
        }
      }

      if (production.size() == 1 && production[0] == "Nil" && prod_index != int32_t(rules.rules[name].productions.size()) - 1)
        release_assert(false && "Nil has to be the last production");

      for (int32_t i = 0; i < int32_t(production.size()); i++)
      {
        if (production[i] != "Nil" && isupper(production[i].str()[0]))
          production[i].setNonTerminal(rules.rules[production[i].str()]);
      }
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