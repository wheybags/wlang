#include <algorithm>
#include "Grammar.hpp"
#include "TestGrammar.hpp"
#include "ParserGenerator.hpp"
#include "../Common/Filesystem.hpp"

const char* wlangGrammarStr = R"STR(
  Root <{Root*}> =
    {{
      Root* rootNode = makeNode<Root>();
      rootNode->funcList = makeNode<FuncList>();

      rootNode->funcList->scope = makeNode<Scope>();
      pushScope(rootNode->funcList->scope);

    }} FuncList<{rootNode->funcList}> $End
    {{
      popScope();
      return rootNode;
    }};


  FuncList <{void}> <{FuncList* funcList}>  =
    Func
    {{
      funcList->functions.emplace_back(v0);
      funcList->scope->functions.insert_or_assign(v0->name, Scope::Item<Func*>{.item = v0, .chunk = &ast});
    }}
    FuncList'<{funcList}>
  |
    ExternFuncDeclaration ";"
    {{
      funcList->functions.emplace_back(v0);
      funcList->scope->functions.insert_or_assign(v0->name, Scope::Item<Func*>{.item = v0, .chunk = &ast});
    }}
    FuncList'<{funcList}>
  |
    "class" $Id
    {{
      Type* type = makeNode<Type>();
      Class* newClass = makeNode<Class>();
      newClass->memberScope = makeNode<Scope>();
      newClass->memberScope->parent = getScope();
      newClass->type = type;
      type->typeClass = newClass;
      type->name = v0;
      funcList->classes.emplace_back(newClass);
      funcList->scope->types.insert_or_assign(type->name, Scope::Item<Type*>{.item = type, .chunk = &ast});
    }}
    "{" ClassMemberList<{newClass}> "}"
    FuncList'<{funcList}>
  ;

  ClassMemberList <{void}> <{Class* newClass}> =
    Type $Id TheRestOfADeclaration
    {{
      VariableDeclaration* variableDeclaration = makeNode<VariableDeclaration>();
      variableDeclaration->type = v0;
      variableDeclaration->name = v1;
      variableDeclaration->initialiser = v2;
      newClass->memberVariables.push_back(variableDeclaration);
      newClass->memberScope->variables.emplace(variableDeclaration->name, Scope::Item<VariableDeclaration*>{.item = variableDeclaration, .chunk = &ast});
    }}
    ";" ClassMemberList<{newClass}>
  |
    Nil
  ;

  FuncList' <{void}> <{FuncList* funcList}> =
    FuncList<{funcList}> | Nil;


  ExternFuncDeclaration <{Func*}> =
    "extern" Type $Id
    {{
      Func* func = makeNode<Func>();
      func->returnType = v0;
      func->name = std::move(v1);
      func->external = true;
    }}
    "(" ArgList<{func}> ")"
    {{ return func; }}
  ;


  Func <{Func*}> =
    Type $Id
    {{
      Func* func = makeNode<Func>();
      func->argsScope = makeNode<Scope>();
      func->returnType = v0;
      func->name = std::move(v1);
    }}
    "(" ArgList<{func}> ")" Block
    {{
      func->funcBody = v2;
      func->funcBody->scope->parent2 = func->argsScope;
      return func;
    }}
  ;

  Block <{Block*}> =
    "{"
    {{
      Block* block = makeNode<Block>();
      block->scope = makeNode<Scope>();
      block->scope->parent = getScope();
      pushScope(block->scope);
    }}
    StatementList<{block}> "}"
    {{
      popScope();
      return block;
    }}
  ;


  StatementList <{void}> <{Block* block}> =
    Statement {{ block->statements.push_back(v0); }} StatementList'<{block}>;


  StatementList' <{void}> <{Block* block}> =
    StatementList<{block}> | Nil;


  Statement <{Statement*}>
    {{ Statement* statement = makeNode<Statement>(); }}
  =
    {{
      ReturnStatement* returnStatement = makeNode<ReturnStatement>();
      IntermediateExpression intermediate;
    }}
    "return" Expression<{intermediate}> ";"
    {{
      returnStatement->retval = resolveIntermediateExpression(std::move(intermediate));
      *statement = returnStatement;
    }}
  |
    {{ IfElseChain* ifElseChain = makeNode<IfElseChain>(); }}
    "if" TheRestOfAnIf<{*ifElseChain}>
    {{ *statement = ifElseChain; }}
  |
    // declaration, or an expression that starts with $Id
    // This awkwardness exists because declarations (and assignments) are not expressions, which is a design choice
    // We basically need to copy paste the rules from Expression into Statement, so we can allow any expression as a statement
    {{ SourceRange idSource = peek().source; }}
    $Id StatementThatStartsWithId <{v0, idSource, statement}> ";"
  |
    // statement that starts with an expression that starts with integer
    $IntegerToken
    {{
      Expression* partial = makeNode<Expression>();
      partial->val = IntegerConstant { .val = v0.val, .size = v0.size };
      partial->source = lastPopped().source;
      IntermediateExpression intermediate;
      intermediate.emplace_back(partial, partial->source);
    }}
    Expression'<{intermediate}> TheRestOfAStatement<{std::move(intermediate), statement}> ";"
  |
    // statement that starts with an expression that starts with logical not (why would you want this? dunno)
    "!"
    {{
      IntermediateExpression intermediate;
      intermediate.emplace_back(Op::Type::LogicalNot, lastPopped().source);
    }}
    Expression<{intermediate}> TheRestOfAStatement<{std::move(intermediate), statement}> ";"
  |
    // statement that starts with an expression that starts with unary minus (again, why would you want this?)
    "-"
    {{
      IntermediateExpression intermediate;
      intermediate.emplace_back(Op::Type::UnaryMinus, lastPopped().source);
    }}
    Expression<{intermediate}> TheRestOfAStatement<{std::move(intermediate), statement}> ";"
  |
    // statement that starts with an expression that starts with false (again, why would you want this?)
    "false"
    {{
      Expression* partial = makeNode<Expression>();
      partial->val = false;
      partial->source = lastPopped().source;
      IntermediateExpression intermediate;
      intermediate.emplace_back(partial, partial->source);
    }}
    Expression'<{intermediate}> TheRestOfAStatement<{std::move(intermediate), statement}> ";"
  |
    // statement that starts with an expression that starts with true (again, why would you want this?)
    "true"
    {{
      Expression* partial = makeNode<Expression>();
      partial->val = true;
      partial->source = lastPopped().source;
      IntermediateExpression intermediate;
      intermediate.emplace_back(partial, partial->source);
    }}
    Expression'<{intermediate}> TheRestOfAStatement<{std::move(intermediate), statement}> ";"
  ;
    {{ return statement; }}


  TheRestOfAnIf <{void}> <{IfElseChain& chain}>
  =
    {{ IntermediateExpression intermediate; }}
    "(" Expression<{intermediate}> ")" Block
    {{
      IfElseChainItem* item = makeNode<IfElseChainItem>();
      item->condition = resolveIntermediateExpression(std::move(intermediate));
      item->block = v0;
      chain.items.emplace_back(item);
    }}
    TheRestOfAnIf'<{chain}>
  ;


  TheRestOfAnIf' <{void}> <{IfElseChain& chain}>
  =
    "else" TheRestOfAnIf''<{chain}>
  |
    Nil
  ;


  TheRestOfAnIf'' <{void}> <{IfElseChain& chain}>
  =
    "if" TheRestOfAnIf<{chain}>
  |
    Block
    {{
      IfElseChainItem* item = makeNode<IfElseChainItem>();
      item->block = v0;
      chain.items.emplace_back(item);
    }}
  ;


  StatementThatStartsWithId <{void}> <{const std::string& id, SourceRange idSource, Statement* statement}> =
    // declaration
    {{
      VariableDeclaration* variableDeclaration = makeNode<VariableDeclaration>();
      variableDeclaration->type = TypeRef { .id = id };
    }}
    Type'<{variableDeclaration->type}>
    $Id TheRestOfADeclaration
    {{
      SourceRange declarationEnd = lastPopped().source;

      variableDeclaration->name = v0;
      variableDeclaration->initialiser = v1;
      variableDeclaration->source = SourceRange(idSource.start, declarationEnd.end);

      getScope()->variables.insert_or_assign(variableDeclaration->name, Scope::Item<VariableDeclaration*>{.item = variableDeclaration, .chunk = &ast});
      *statement = variableDeclaration;
    }}
  |
    // assign, or standalone expression that starts with id
    {{
      Expression* partial = makeNode<Expression>();
      partial->val = ScopeId(id);
      partial->source = idSource;

      IntermediateExpression intermediate;
      intermediate.emplace_back(partial, partial->source);
    }}
    // The statement A*b is ambiguous - is it a multiplication or pointer declaration?
    // Here we resolve the ambiguity - it's a pointer declaration. We do this by using a special version
    // of the Expression' rule that has no multiply operator.
    // Maybe one day I will add a fancy selection based on whether A exists as a variable, but TBH it's a kinda
    // useless construct so I probably won't.
    Expression'NoMul<{intermediate}>
    TheRestOfAStatement<{std::move(intermediate), statement}>
  |
    Nil
    {{
      Expression* expression = makeNode<Expression>();
      expression->val = ScopeId(id);
      *statement = expression;
    }}
  ;


  TheRestOfADeclaration <{Expression*}> =
    {{ IntermediateExpression intermediateExpression; }}
    "=" Expression<{intermediateExpression}>
    {{ return resolveIntermediateExpression(std::move(intermediateExpression)); }}
  |
    Nil
    {{ return nullptr; }}
  ;


  TheRestOfAStatement <{void}> <{IntermediateExpression intermediateExpression, Statement* statement}> =
    "="
    {{
      IntermediateExpression intermediateR;
    }}
    Expression<{intermediateR}>
    {{
      Assignment* assignment = makeNode<Assignment>();
      assignment->left = resolveIntermediateExpression(std::move(intermediateExpression));
      assignment->right = resolveIntermediateExpression(std::move(intermediateR));
      *statement = assignment;
    }}
  |
    Nil
    {{ *statement = resolveIntermediateExpression(std::move(intermediateExpression)); }};


  Expression <{void}> <{IntermediateExpression& result}>
     {{ SourceRange source = peek().source; }}
  =
    $Id
    {{
      Expression* expression = makeNode<Expression>();
      expression->val = ScopeId(v0);
      expression->source = source;
      result.emplace_back(expression, expression->source);
    }} Expression'<{result}>
  |
    $IntegerToken
    {{
      Expression* expression = makeNode<Expression>();
      expression->val = IntegerConstant { .val = v0.val, .size = v0.size };
      expression->source = source;
      result.emplace_back(expression, expression->source);
    }} Expression'<{result}>
  |
    "false"
    {{
      Expression* expression = makeNode<Expression>();
      expression->val = false;
      expression->source = source;
      result.emplace_back(expression, expression->source);
    }} Expression'<{result}>
  |
    "true"
    {{
      Expression* expression = makeNode<Expression>();
      expression->val = true;
      expression->source = source;
      result.emplace_back(expression, expression->source);
    }} Expression'<{result}>
  |
    "!"
    {{
      result.emplace_back(Op::Type::LogicalNot, source);
    }} Expression<{result}>
  |
    "-"
    {{
      result.emplace_back(Op::Type::UnaryMinus, source);
    }} Expression<{result}>
  ;


  Expression'NoMul <{void}> <{IntermediateExpression& result}> =
    {{ result.emplace_back(Op::Type::CompareEqual, peek().source); }}
    "==" Expression<{result}>
  |
    {{ result.emplace_back(Op::Type::CompareNotEqual, peek().source); }}
    "!=" Expression<{result}>
  |
    {{ result.emplace_back(Op::Type::LogicalAnd, peek().source); }}
    "&&" Expression<{result}>
  |
    {{ result.emplace_back(Op::Type::LogicalOr, peek().source); }}
    "||" Expression<{result}>
  |
    {{ result.emplace_back(Op::Type::Add, peek().source); }}
    "+" Expression<{result}>
  |
    {{ result.emplace_back(Op::Type::Subtract, peek().source); }}
    "-" Expression<{result}>
  |
    {{ result.emplace_back(Op::Type::Divide, peek().source); }}
    "/" Expression<{result}>
  |
    {{ result.emplace_back(Op::Type::MemberAccess, peek().source); }}
    "." Expression<{result}>
  |
    {{
      result.emplace_back(Op::Type::Call, peek().source);
      std::vector<Expression*> argList;
      SourceRange openBracket = peek().source;
     }}
    "("
     CallParamList<{argList}>
    {{
      SourceRange closeBracket = peek().source;
      result.emplace_back(std::move(argList), SourceRange(openBracket.start, closeBracket.end));
    }}
    ")"
  |
    {{
      IntermediateExpression intermediateExpression;
      result.emplace_back(Op::Type::Subscript, peek().source);
      SourceRange openBracket = peek().source;
    }}
    "[" Expression<{intermediateExpression}> "]"
    {{
      SourceRange closeBracket = peek().source;
      Expression* index = resolveIntermediateExpression(std::move(intermediateExpression));
      result.emplace_back(index, SourceRange(openBracket.start, closeBracket.end));
    }}
  |
    Nil;


  Expression' <{void}> <{IntermediateExpression& result}> =
    {{ result.emplace_back(Op::Type::Multiply, peek().source); }}
    "*" Expression<{result}>
  |
    Expression'NoMul<{result}>
  ;

  CallParamList <{void}> <{std::vector<Expression*>& argList}> =
    {{ IntermediateExpression intermediateExpression; }}
    Expression<{intermediateExpression}>
    {{ argList.emplace_back(resolveIntermediateExpression(std::move(intermediateExpression))); }}
    CallParamList'<{argList}>
  |
    Nil;

  CallParamList' <{void}> <{std::vector<Expression*>& argList}> =
    "," CallParamList<{argList}>
  |
    Nil;

  Type <{TypeRef}> =
    $Id
    {{ TypeRef retval{ .id = v0 }; }}
    Type'<{retval}>
    {{ return retval; }}
  ;

  Type' <{void}> <{TypeRef& typeRef}> =
    "*" {{ typeRef.pointerDepth++; }} Type'<{typeRef}>
  |
    Nil;


  ArgList <{void}> <{Func* func}> =
    Arg
    {{
      func->args.emplace_back(v0);
      if (func->argsScope)
        func->argsScope->variables.insert_or_assign(v0->name, Scope::Item<VariableDeclaration*>{.item = v0, .chunk = &ast});
    }}
    ArgList'<{func}>
  |
    Nil
  ;

  ArgList' <{void}> <{Func* func}> =
    "," ArgList<{func}>
  |
    Nil
  ;

  Arg <{VariableDeclaration*}> =
    Type $Id
    {{
      VariableDeclaration* arg = makeNode<VariableDeclaration>();
      arg->type = v0;
      arg->name = v1;
      return arg;
    }};
)STR";



void dumpFirstFollows(Grammar& grammar)
{
  auto pad = [&](const std::string& name)
  {
    int32_t max_len = 0;
    for (const auto& pair: grammar.getRules())
      max_len = std::max(int32_t(pair.first.size()), max_len);

    std::string retval;
    for (int32_t i = 0; i < (max_len - int32_t(name.size())); i++)
      retval += ' ';

    return retval;
  };


  printf("Firsts:\n");
  for (const std::string& name : grammar.getKeys())
  {
    std::string line = "    " + name + " " + pad(name) + "= ";

    for (const std::vector<std::string>& prod_firsts : grammar.first(name))
    {
      for (const std::string& item : prod_firsts)
        line += item + " ";
      line += "| ";
    }

    if (!line.empty())
      line.resize(line.size() - 3);

    puts(line.c_str());
  }

  printf("\nFollows:\n");
  for (const std::string& name : grammar.getKeys())
  {
    std::string line = "    " + name + " " + pad(name) + "= ";

    std::unordered_set<std::string> follows = grammar.follow(name);
    std::vector<std::string> sortedFollows(follows.begin(), follows.end());
    std::sort(sortedFollows.begin(), sortedFollows.end());

    for (const std::string& item : sortedFollows)
      line += item + " ";

    if (!line.empty())
      line.resize(line.size() - 1);

    puts(line.c_str());
  }
}

int main()
{
  test();

  Grammar wlangGrammar(wlangGrammarStr);

  dumpFirstFollows(wlangGrammar);

  std::filesystem::path rootPath = getPathToThisExecutable();
  while (!std::filesystem::exists(rootPath / "src" / "Parser.cpp"))
    rootPath = rootPath.parent_path();

  std::filesystem::path parserImplementationPath = rootPath / "src" / "ParserRules.inl";
  std::filesystem::path parserDeclarationPath = rootPath / "src" / "ParserRulesDeclarations.inl";

  ParserSource parserSource = generateParser(wlangGrammar);

  release_assert(overwriteFileWithString(parserImplementationPath, parserSource.implementationSource));
  release_assert(overwriteFileWithString(parserDeclarationPath, parserSource.declarationSource));

  return 0;
}