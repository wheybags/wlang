// PARSE Root BEGIN
Root*
Parser::parseRoot(
  ParseContext& ctx)
{
  Root* rootNode = makeNode<Root>();
  rootNode->rootScope = makeNode<Scope>();
  ctx.pushScope(rootNode->rootScope);
  rootNode->funcList = makeNode<FuncList>();

  parseFuncList(rootNode->funcList, ctx);
  release_assert(ctx.popCheck(TT::End));

  return rootNode;
}
//END OF PARSE Root

// PARSE FuncList BEGIN
void
Parser::parseFuncList(
  FuncList* funcList,
  ParseContext& ctx)
{
  Func* v0 = parseFunc(ctx);
  funcList->functions.emplace_back(v0);
  parseFuncListP(funcList, ctx);
}
//END OF PARSE FuncList

// PARSE FuncList' BEGIN
void
Parser::parseFuncListP(
  FuncList* funcList,
  ParseContext& ctx)
{
  if (ctx.peekCheck(TT::Id))
  {
    return parseFuncList(funcList, ctx);
  }
  else
  {
    release_assert(FuncListPFollow.count(ctx.peek().type));
  }
}
//END OF PARSE FuncList'

// PARSE Func BEGIN
Func*
Parser::parseFunc(
  ParseContext& ctx)
{
  Func* func = makeNode<Func>();
  func->returnType = parseType(nullptr, ctx);
  func->name = parseId(ctx);
  release_assert(ctx.popCheck(TT::OpenBracket));
  parseFuncP(func, ctx);
  return func;
}
//END OF PARSE Func

// PARSE Func' BEGIN
void
Parser::parseFuncP(
  Func* func,
  ParseContext& ctx)
{
  if (ctx.peekCheck(TT::Id))
  {
    func->argList = parseArgList(ctx);
    release_assert(ctx.popCheck(TT::CloseBracket));
    func->funcBody = parseBlock(ctx);
  }
  else if (ctx.peekCheck(TT::CloseBracket))
  {
    release_assert(ctx.popCheck(TT::CloseBracket));
    func->funcBody = parseBlock(ctx);
  }
}
//END OF PARSE Func'

// PARSE Block BEGIN
Block*
Parser::parseBlock(
  ParseContext& ctx)
{
  Block* block = makeNode<Block>();
  release_assert(ctx.popCheck(TT::OpenBrace));
  parseStatementList(block, ctx);
  release_assert(ctx.popCheck(TT::CloseBrace));
  return block;
}
//END OF PARSE Block

// PARSE StatementList BEGIN
void
Parser::parseStatementList(
  Block* block,
  ParseContext& ctx)
{
  block->statements.push_back(parseStatement(ctx));
  release_assert(ctx.popCheck(TT::Semicolon));
  parseStatementListP(block, ctx);
}
//END OF PARSE StatementList

// PARSE StatementList' BEGIN
void
Parser::parseStatementListP(
  Block* block,
  ParseContext& ctx)
{
  if (ctx.peekCheck(TT::Return) || ctx.peekCheck(TT::Id))
  {
    parseStatementList(block, ctx);
  }
  else
  {
    // Nil
    release_assert(StatementListPFollow.count(ctx.peek().type));
  }
}
//END OF PARSE StatementList'

// PARSE Statement BEGIN
Statement*
Parser::parseStatement(
  ParseContext& ctx)
{
  Statement* statement = makeNode<Statement>();
  if (ctx.peekCheck(TT::Return))
  {
    ctx.pop();
    ReturnStatement* returnStatement = makeNode<ReturnStatement>();

    IntermediateExpression intermediate;
    parseExpression(intermediate, ctx);
    returnStatement->retval = resolveIntermediateExpression(std::move(intermediate));

    *statement = returnStatement;
  }
  else if(ctx.peekCheck(TT::Id))
  {
    Id* id = parseId(ctx);
    parseStatementP(id, statement, ctx);
  }
  else
  {

  }

  return statement;
}
//END OF PARSE Statement

// PARSE Statement' BEGIN
void
Parser::parseStatementP(
  Id* id,
  Statement* statement,
  ParseContext& ctx)
{
  if (ctx.peekCheck(TT::Id))
  {
    VariableDeclaration* variableDeclaration = makeNode<VariableDeclaration>();
    variableDeclaration->type = parseType(id, ctx);
    variableDeclaration->name = parseId(ctx);

//    auto it = ctx.getScope()->variables.find(variableDeclaration->name->name);
//    release_assert(it == ctx.getScope()->variables.end());
//    ctx.getScope()->variables.emplace_hint(it, variableDeclaration->name->name, variableDeclaration);

    *statement = variableDeclaration;
  }
  else if (ctx.peekCheck(TT::CompareEqual) || ctx.peekCheck(TT::LogicalAnd) || ctx.peekCheck(TT::Assign))
  {
    Assignment* assignment = makeNode<Assignment>();
    {
      Expression* partial = makeNode<Expression>();
      *partial = id;
      IntermediateExpression intermediate;
      intermediate.push_back(partial);
      parseExpressionP(intermediate, ctx);
      assignment->left = resolveIntermediateExpression(std::move(intermediate));
    }
    release_assert(ctx.popCheck(TT::Assign));
    {
      IntermediateExpression intermediate;
      parseExpression(intermediate, ctx);
      assignment->right = resolveIntermediateExpression(std::move(intermediate));
    }
    *statement = assignment;
  }
  else
  {
    message_and_abort("bad statement");
  }
}
//END OF PARSE Statement'

// PARSE Expression BEGIN
void
Parser::parseExpression(
  IntermediateExpression& result,
  ParseContext& ctx)
{
  if (ctx.peekCheck(TT::Id))
  {
    Expression* expression = makeNode<Expression>();
    *expression = parseId(ctx);
    result.push_back(expression);
    parseExpressionP(result, ctx);
  }
  else if (ctx.peekCheck(TT::Int32))
  {
    Expression* expression = makeNode<Expression>();
    *expression = ctx.pop().i32Value;
    result.push_back(expression);
    parseExpressionP(result, ctx);
  }
  else
  {
    message_and_abort("bad expression");
  }
}
//END OF PARSE Expression

// PARSE Expression' BEGIN
void
Parser::parseExpressionP(
  IntermediateExpression& result,
  ParseContext& ctx)
{
  if (ctx.peekCheck(TT::CompareEqual))
  {
    ctx.pop();
    result.push_back(Op::Type::CompareEqual);
    parseExpression(result, ctx);
  }
  else if (ctx.peekCheck(TT::LogicalAnd))
  {
    ctx.pop();
    result.push_back(Op::Type::LogicalAnd);
    parseExpression(result, ctx);
  }
  else
  {
    // Nil
    release_assert(ExpressionPFollow.count(ctx.peek().type));
  }
}
//END OF PARSE Expression'

// PARSE Type BEGIN
Type*
Parser::parseType(
  Id* fromId,
  ParseContext& ctx)
{
  const std::string* typeName = nullptr;

  if (fromId)
  {
    typeName = &fromId->name;
  }
  else
  {
    const Token& typeToken = ctx.pop();
    release_assert(typeToken.type == Token::Type::Id);
    typeName = &typeToken.idValue;
  }

  auto it = types.find(*typeName);

  if (it == types.end())
  {
    Type* type = makeNode<Type>();
    type->name = *typeName;
    types.emplace_hint(it, *typeName, type);
    return type;
  }
  else
  {
    return it->second;
  }
}
//END OF PARSE Type

// PARSE ArgList BEGIN
ArgList*
Parser::parseArgList(
  ParseContext& ctx)
{
  ArgList* argList = makeNode<ArgList>();
  argList->arg = parseArg(ctx);
  parseArgListP(argList, ctx);
  return argList;
}
//END OF PARSE ArgList

// PARSE ArgList' BEGIN
void
Parser::parseArgListP(
  ArgList* argList,
  ParseContext& ctx)
{
  if (ctx.peekCheck(TT::Comma))
  {
    ctx.pop();
    argList->next = parseArgList(ctx);
  }
  else
  {
    release_assert(ctx.peekCheck(TT::CloseBracket));
  }
}
//END OF PARSE ArgList'

// PARSE Arg BEGIN
Arg*
Parser::parseArg(
  ParseContext& ctx)
{
  Arg *arg = makeNode<Arg>();
  arg->type = parseType(nullptr, ctx);
  arg->name = parseId(ctx);
  return arg;
}
//END OF PARSE Arg

