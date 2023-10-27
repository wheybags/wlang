#include <string>
#include "Grammar.hpp"
#include "../StringUtil.hpp"

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

      auto handleSourceInsert = [&](const std::string& sourceInsert)
      {
        int32_t xStart = 0;
        {
          for (int32_t i = 0; i < int32_t(sourceInsert.size()); i++)
          {
            if (sourceInsert[i] == '\n')
            {
              xStart = 0;
              continue;
            }

            if (!Str::isSpace(sourceInsert[i]))
              break;
            xStart++;
          }
        }

        int32_t linesSent = 0;

        std::string line;
        bool allSpace = true;
        int32_t charsOnLine = 0;

        for (char c : sourceInsert)
        {
          if (c == '\n')
          {
            if (linesSent > 0 || !allSpace)
              appendSourceLine(line);

            linesSent++;
            line.clear();
            allSpace = true;
            charsOnLine = 0;
            continue;
          }

          if (!Str::isSpace(c))
            allSpace = false;

          if (charsOnLine >= xStart || !Str::isSpace(c))
            line += c;

          charsOnLine++;
        }

        if (!line.empty() && allSpace)
          appendSourceLine(line);
      };

      int32_t nonTerminalIndex = 0;
      for (int32_t productionIndex = 0; productionIndex < int32_t(production.size()); productionIndex++)
      {
        const ProductionItem& item = production[productionIndex];

        if (item == "Nil")
            continue;

        if (!item.codeInsertBefore.empty())
        {
          if (productionIndex != 0)
            appendSourceLine("");

          handleSourceInsert(item.codeInsertBefore);

          if (productionIndex != int32_t(production.size())-1)
            appendSourceLine("");
        }

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

        if (!item.codeInsertAfter.empty())
        {
          if (productionIndex != 0)
            appendSourceLine("");
          handleSourceInsert(item.codeInsertAfter);
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
  }

  return parserSource;
}