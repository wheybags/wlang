Root* Parser::parseRoot(ParseContext& ctx)
{
  if (ctx.peekCheck(TT::Id))
  {
    Root* rootNode = makeNode<Root>();
    rootNode->rootScope = makeNode<Scope>();
    ctx.pushScope(rootNode->rootScope);
    rootNode->funcList = makeNode<FuncList>();
    
    parseFuncList(ctx, rootNode->funcList);
    release_assert(ctx.popCheck(TT::End));
    
    return rootNode;
  }
  else
  {
    release_assert(false);
  }
}

void Parser::parseFuncList(ParseContext& ctx, FuncList* funcList)
{
  if (ctx.peekCheck(TT::Id))
  {
    Func* v0 = parseFunc(ctx);
    
    funcList->functions.emplace_back(v0);
    
    parseFuncListP(ctx, funcList);
  }
  else
  {
    release_assert(false);
  }
}

void Parser::parseFuncListP(ParseContext& ctx, FuncList* funcList)
{
  if (ctx.peekCheck(TT::Id))
  {
    parseFuncList(ctx, funcList);
  }
  else
  {
    release_assert(ctx.peekCheck(TT::End));
  }
}

Func* Parser::parseFunc(ParseContext& ctx)
{
  if (ctx.peekCheck(TT::Id))
  {
    Type* v0 = parseType(ctx);
    std::string v1 = parseId(ctx);
    
    Func* func = makeNode<Func>();
    func->returnType = v0;
    func->name = std::move(v1);
    
    release_assert(ctx.popCheck(TT::OpenBracket));
    parseFuncP(ctx, func);
    
    return func; 
  }
  else
  {
    release_assert(false);
  }
}

void Parser::parseFuncP(ParseContext& ctx, Func* func)
{
  if (ctx.peekCheck(TT::Id))
  {
    ArgList* v0 = parseArgList(ctx);
    
    func->argList = v0; 
    
    release_assert(ctx.popCheck(TT::CloseBracket));
    Block* v1 = parseBlock(ctx);
    
    func->funcBody = v1; 
  }
  else if (ctx.peekCheck(TT::CloseBracket))
  {
    release_assert(ctx.popCheck(TT::CloseBracket));
    Block* v0 = parseBlock(ctx);
    
    func->funcBody = v0; 
  }
  else
  {
    release_assert(false);
  }
}

Block* Parser::parseBlock(ParseContext& ctx)
{
  if (ctx.peekCheck(TT::OpenBrace))
  {
    release_assert(ctx.popCheck(TT::OpenBrace));
    
    Block* block = makeNode<Block>(); 
    
    parseStatementList(ctx, block);
    release_assert(ctx.popCheck(TT::CloseBrace));
    
    return block; 
  }
  else
  {
    release_assert(false);
  }
}

void Parser::parseStatementList(ParseContext& ctx, Block* block)
{
  if (ctx.peekCheck(TT::Return) || ctx.peekCheck(TT::Id))
  {
    Statement* v0 = parseStatement(ctx);
    
    block->statements.push_back(v0); 
    
    release_assert(ctx.popCheck(TT::Semicolon));
    parseStatementListP(ctx, block);
  }
  else
  {
    release_assert(false);
  }
}

void Parser::parseStatementListP(ParseContext& ctx, Block* block)
{
  if (ctx.peekCheck(TT::Return) || ctx.peekCheck(TT::Id))
  {
    parseStatementList(ctx, block);
  }
  else
  {
    release_assert(ctx.peekCheck(TT::CloseBrace));
  }
}

Statement* Parser::parseStatement(ParseContext& ctx)
{
  if (ctx.peekCheck(TT::Return))
  {
    Statement* statement = makeNode<Statement>();
    ReturnStatement* returnStatement = makeNode<ReturnStatement>();
    IntermediateExpression intermediate;
    
    release_assert(ctx.popCheck(TT::Return));
    parseExpression(ctx, intermediate);
    
    returnStatement->retval = resolveIntermediateExpression(std::move(intermediate));
    *statement = returnStatement;
    return statement;
  }
  else if (ctx.peekCheck(TT::Id))
  {
    Statement* statement = makeNode<Statement>(); 
    
    std::string v0 = parseId(ctx);
    parseStatementP(ctx, v0, statement);
    
    return statement; 
  }
  else
  {
    release_assert(false);
  }
}

void Parser::parseStatementP(ParseContext& ctx, const std::string& id, Statement* statement)
{
  if (ctx.peekCheck(TT::Id))
  {
    std::string v0 = parseId(ctx);
    VariableDeclaration* variableDeclaration = makeNode<VariableDeclaration>();
    variableDeclaration->type = getType(id);
    variableDeclaration->name = v0;
    *statement = variableDeclaration;
  }
  else if (ctx.peekCheck(TT::CompareEqual) || ctx.peekCheck(TT::LogicalAnd) || ctx.peekCheck(TT::Assign))
  {
    Expression* partial = makeNode<Expression>();
    *partial = id;
    IntermediateExpression intermediateL;
    intermediateL.push_back(partial);
    
    parseExpressionP(ctx, intermediateL);
    release_assert(ctx.popCheck(TT::Assign));
    
    IntermediateExpression intermediateR;
    
    parseExpression(ctx, intermediateR);
    
    Assignment* assignment = makeNode<Assignment>();
    assignment->left = resolveIntermediateExpression(std::move(intermediateL));
    assignment->right = resolveIntermediateExpression(std::move(intermediateR));
    *statement = assignment;
  }
  else
  {
    release_assert(false);
  }
}

void Parser::parseExpression(ParseContext& ctx, IntermediateExpression& result)
{
  if (ctx.peekCheck(TT::Id))
  {
    std::string v0 = parseId(ctx);
    
    Expression* expression = makeNode<Expression>();
    *expression = v0;
    result.push_back(expression);
    
    parseExpressionP(ctx, result);
  }
  else if (ctx.peekCheck(TT::Int32))
  {
    int32_t v0 = parseInt32(ctx);
    
    Expression* expression = makeNode<Expression>();
    *expression = v0;
    result.push_back(expression);
    
    parseExpressionP(ctx, result);
  }
  else
  {
    release_assert(false);
  }
}

void Parser::parseExpressionP(ParseContext& ctx, IntermediateExpression& result)
{
  if (ctx.peekCheck(TT::CompareEqual))
  {
    result.push_back(Op::Type::CompareEqual); 
    
    release_assert(ctx.popCheck(TT::CompareEqual));
    parseExpression(ctx, result);
  }
  else if (ctx.peekCheck(TT::LogicalAnd))
  {
    result.push_back(Op::Type::LogicalAnd); 
    
    release_assert(ctx.popCheck(TT::LogicalAnd));
    parseExpression(ctx, result);
  }
  else
  {
    release_assert(ctx.peekCheck(TT::Assign) || ctx.peekCheck(TT::Semicolon));
  }
}

Type* Parser::parseType(ParseContext& ctx)
{
  if (ctx.peekCheck(TT::Id))
  {
    std::string v0 = parseId(ctx);
    return getType(v0); 
  }
  else
  {
    release_assert(false);
  }
}

ArgList* Parser::parseArgList(ParseContext& ctx)
{
  if (ctx.peekCheck(TT::Id))
  {
    Arg* v0 = parseArg(ctx);
    
    ArgList* argList = makeNode<ArgList>();
    argList->arg = v0;
    
    parseArgListP(ctx, argList);
    
    return argList; 
  }
  else
  {
    release_assert(false);
  }
}

void Parser::parseArgListP(ParseContext& ctx, ArgList* argList)
{
  if (ctx.peekCheck(TT::Comma))
  {
    release_assert(ctx.popCheck(TT::Comma));
    ArgList* v0 = parseArgList(ctx);
    
    argList->next = v0; 
  }
  else
  {
    release_assert(ctx.peekCheck(TT::CloseBracket));
  }
}

Arg* Parser::parseArg(ParseContext& ctx)
{
  if (ctx.peekCheck(TT::Id))
  {
    Type* v0 = parseType(ctx);
    std::string v1 = parseId(ctx);
    
    Arg* arg = makeNode<Arg>();
    arg->type = v0;
    arg->name = v1;
    return arg;
  }
  else
  {
    release_assert(false);
  }
}

