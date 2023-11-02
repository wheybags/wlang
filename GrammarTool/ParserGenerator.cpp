#include "ParserGenerator.hpp"
#include <string>
#include "Grammar.hpp"
#include "../StringUtil.hpp"

ParserSource generateParser(const Grammar& grammar)
{
  ParserSource parserSource;

  int32_t tabIndex = 0;
  auto appendSourceLine = [&](std::string_view line)
  {
    int32_t openBraceCount = int32_t(std::count(line.begin(), line.end(), '{'));
    int32_t closeBraceCount = int32_t(std::count(line.begin(), line.end(), '}'));

    int32_t tabDelta = openBraceCount - closeBraceCount;

    if (tabDelta < 0)
      tabIndex += tabDelta;

    for (int32_t i = 0; i < tabIndex * 2; i++)
      parserSource.implementationSource += ' ';

    parserSource.implementationSource += line;
    parserSource.implementationSource += '\n';

    if (tabDelta > 0)
      tabIndex += tabDelta;
  };

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

    if (!line.empty() && !allSpace)
      appendSourceLine(line);
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
    {"\"!=\"", "CompareNotEqual"},
    {"\"&&\"", "LogicalAnd"},
    {"\"||\"", "LogicalOr"},
    {"\"!\"", "LogicalNot"},
    {"\"+\"", "Add"},
    {"\"-\"", "Subtract"},
    {"\"*\"", "Asterisk"},
    {"\"/\"", "Divide"},
    {"\"return\"", "Return"},
    {"\";\"", "Semicolon"},
    {"\"=\"", "Assign"},
    {"\"class\"", "Class"},
    {"\".\"", "Dot"},
    {"\"if\"", "If"},
    {"\"else\"", "Else"},
    {"$End", "End"},
  };

  std::unordered_map<std::string, std::string> idTokenMapping
  {
    {"$Id", "std::string"},
    {"$Int32", "int32_t"},
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

    // function declaration
    {
      std::string returnType = "void";
      if (!rule.returnType.empty())
        returnType = rule.returnType;

      std::string argsList = "ParseContext& ctx";
      if (!rule.arguments.empty())
        argsList += ", " + rule.arguments;

      appendSourceLine(returnType + " Parser::parse" + sanitiseName(name) + "(" + argsList + ")");

      parserSource.declarationSource += returnType + " parse" + sanitiseName(name) + "(" + argsList + ");\n";
    }
    appendSourceLine("{");

    if (!rule.codeInsertBefore.empty())
    {
      handleSourceInsert(rule.codeInsertBefore);
      appendSourceLine("");
    }

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

      if (i > 0)
        appendSourceLine("else if (" + firstsCheck + ")");
      else
        appendSourceLine("if (" + firstsCheck + ")");
      appendSourceLine("{");

      int32_t variableIndex = 0;
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

          appendSourceLine("");
        }

        if (item.isStr())
        {
          auto it = idTokenMapping.find(item.str());
          if (it != idTokenMapping.end())
          {
            std::string callLine = it->second + " v" + std::to_string(variableIndex) + " = ";
            variableIndex++;

            callLine += "parse" + tokenTypeMapping.at(item.str()) + "(ctx);";
            appendSourceLine(callLine);
          }
          else
          {
            appendSourceLine("release_assert(ctx.popCheck(TT::" + tokenTypeMapping.at(item.str()) + "));");
          }
        }
        else
        {
          std::string callLine;
          if (!item.nonTerminal().returnType.empty() && item.nonTerminal().returnType != "void")
          {
            callLine += item.nonTerminal().returnType + " v" + std::to_string(variableIndex) + " = ";
            variableIndex++;
          }

          std::string args = "ctx";
          if (!item.parameters.empty())
            args += ", " + item.parameters;

          callLine += "parse" + sanitiseName(item.nonTerminal().name) + "(" + args + ");";

          appendSourceLine(callLine);
        }

        if (!item.codeInsertAfter.empty())
        {
          if (productionIndex != 0)
            appendSourceLine("");
          handleSourceInsert(item.codeInsertAfter);
        }
      }

      appendSourceLine("}");
    }

    appendSourceLine("else");
    appendSourceLine("{");

    if (grammar.can_be_nil(name))
    {
      // if Nil is direct, then insert code attached to it (if any)
      const Production& lastProduction = rule.productions[rule.productions.size() - 1];
      bool lastProductionIsNil = lastProduction.size() == 1 && lastProduction[0] == "Nil";

      if (lastProductionIsNil && !lastProduction[0].codeInsertBefore.empty())
        handleSourceInsert(lastProduction[0].codeInsertBefore);

      std::unordered_set<std::string> follows = grammar.follow(name);

      std::string followsCheck;
      for (const std::string& item: follows)
        followsCheck += "ctx.peekCheck(TT::" + tokenTypeMapping.at(item) + ") || ";

      release_assert(!followsCheck.empty());
      followsCheck.resize(followsCheck.size() - 4);

      appendSourceLine("release_assert(" + followsCheck + ");");

      if (lastProductionIsNil && !lastProduction[0].codeInsertAfter.empty())
        handleSourceInsert(lastProduction[0].codeInsertAfter);
    }
    else
    {
      appendSourceLine("release_assert(false);");
    }

    appendSourceLine("}");

    if (!rule.codeInsertAfter.empty())
    {
      appendSourceLine("");
      handleSourceInsert(rule.codeInsertAfter);
    }

    appendSourceLine("}");
    appendSourceLine("");
  }

  return parserSource;
}