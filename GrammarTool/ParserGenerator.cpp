#include <string>
#include "Grammar.hpp"

std::string generateParser(const Grammar& grammar)
{
  std::string parserSource;

  int32_t tabIndex = 0;
  auto appendSourceLine = [&](std::string_view line)
  {
    int32_t openBraceCount = int32_t(std::count(line.begin(), line.end(), '{'));
    int32_t closeBraceCount = int32_t(std::count(line.begin(), line.end(), '}'));

    int32_t tabDelta = openBraceCount - closeBraceCount;

    if (tabDelta < 0)
      tabIndex += tabDelta;

    for (int32_t i = 0; i < tabIndex * 2; i++)
      parserSource += ' ';

    parserSource += line;
    parserSource += '\n';

    if (tabDelta > 0)
      tabIndex += tabDelta;
  };

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

    std::string returnType = "void";
    if (!rule.returnType.empty())
      returnType = rule.returnType;

    appendSourceLine(returnType + " Parser::parse" + sanitiseName(name) + "(ParseContext& ctx)");
    appendSourceLine("{");

    std::vector<std::vector<std::string>> productionFirsts = grammar.first(name);

    for (int32_t i = 0; i < int32_t(rule.productions.size()); i++)
    {
      const Production& production = rule.productions[i];
      const std::vector<std::string>& firsts = productionFirsts[i];

      bool onlyOne = rule.productions.size() == 1;

      if (!onlyOne)
      {
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

        if (i > 0)
          appendSourceLine("else if (" + firstsCheck + ")");
        else
          appendSourceLine("if (" + firstsCheck + ")");
        appendSourceLine("{");
      }

      int32_t nonTerminalIndex = 0;
      for (const ProductionItem& item : production)
      {
        if (item == "Nil")
            continue;

        if (item.isStr())
        {
          appendSourceLine("release_assert(ctx.popCheck(TT::" + tokenTypeMapping.at(item.str()) + "));");
        }
        else
        {
          std::string callLine;
          if (!item.nonTerminal().returnType.empty())
            callLine += item.nonTerminal().returnType + " v" + std::to_string(nonTerminalIndex) + " = ";

          callLine += "parse" + sanitiseName(item.nonTerminal().name) + "(ctx);";

          appendSourceLine(callLine);
        }
      }

      if (!onlyOne)
        appendSourceLine("}");
    }

    if (grammar.can_be_nil(name))
    {
      std::unordered_set<std::string> follows = grammar.follow(name);

      appendSourceLine("else");
      appendSourceLine("{");

      std::string followsCheck;
      for (const std::string& item: follows)
        followsCheck += "ctx.peekCheck(TT::" + tokenTypeMapping.at(item) + ") &&";

      release_assert(!followsCheck.empty());
      followsCheck.resize(followsCheck.size() - 3);

      appendSourceLine("release_assert(" + followsCheck + ")");

      appendSourceLine("}");
    }

    appendSourceLine("}");
    appendSourceLine("");


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