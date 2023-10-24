#include "Grammar.hpp"

std::string generateParser(const Grammar& grammar)
{
  std::string parserSource;

  std::unordered_map<std::string, std::string> tokenTypeMapping
  {
    {"$Id", "Id"},
    {"$Int32", "Int32"},
    {"\",\"", "Comma"},
    {"\"(\"", "OpenBracket"},
    {"\")\"", "CloseBracket"},
    {"\"{\"", "OpenBrace"},
    {"\"}\"", "CloseBrace"},
    {"\"==\"", "CompareEqual"},
    {"\"&&\"", "LogicalAnd"},
    {"\"return\"", "Return"},
    {"\";\"", "Semicolon"},
    {"\"=\"", "Assign"},
    {"$End", "End"},
  };

  auto sanitiseName = [](const std::string& ruleName)
  {
    std::string nameSanitised = ruleName;
    for (char& c : nameSanitised)
    {
      if (c == '\'')
        c = 'P';
    }
    return nameSanitised;
  };

  for (const std::string& name: grammar.getKeys())
  {
    const NonTerminal& rule = grammar.getRules().at(name);

    parserSource += "void Parser::parse" + sanitiseName(name) + "(ParseContext& ctx)\n";
    parserSource += "{\n";

    std::vector<std::vector<std::string>> productionFirsts = grammar.first(name);

    for (int32_t i = 0; i < int32_t(rule.productions.size()); i++)
    {
      const Production& production = rule.productions[i];
      const std::vector<std::string>& firsts = productionFirsts[i];

      std::string firstsCheck;
      for (const std::string& item: firsts)
      {
        if (item == "Nil")
          continue;
        firstsCheck += "ctx.peekCheck(TT::" + tokenTypeMapping.at(item) + ") || ";
      }

      if (firstsCheck.empty())
        continue;

      firstsCheck.resize(firstsCheck.size() - 4);

      parserSource += "  ";
      if (i > 0)
        parserSource += "else ";
      parserSource +="if (" + firstsCheck + ")\n";
      parserSource += "  {\n";

      for (const ProductionItem& item : production)
      {
        if (item.isStr())
          parserSource += "    ctx.pop();\n";
        else
          parserSource += "    parse" + sanitiseName(item.nonTerminal().name) + "(ctx);\n";
      }

      parserSource += "  }\n";
    }

    if (grammar.can_be_nil(name))
    {
      std::unordered_set<std::string> follows = grammar.follow(name);

      parserSource += "  else\n";
      parserSource += "  {\n";

      std::string followsCheck;
      for (const std::string& item: follows)
        followsCheck += "ctx.peekCheck(TT::" + tokenTypeMapping.at(item) + ") &&";

      release_assert(!followsCheck.empty());
      followsCheck.resize(followsCheck.size() - 3);

      parserSource += "    release_assert(" + followsCheck + ")\n";

      parserSource += "  }\n";
    }

    parserSource += "}\n\n";



//void Parser::parseFuncListP(FuncList* list, ParseContext& ctx)
//{
//  if (ctx.peekCheck(TT::Id))
//  {
//    list->next = parseFuncList(ctx);
//  }
//  else
//  {
//    // Nil
//    release_assert(FuncListPFollow.count(ctx.peek().type));
//  }
//}
  }

  return parserSource;
}