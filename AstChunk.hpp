#pragma once

#include "Ast.hpp"

class AstChunk
{
public:
  AstChunk() = default;
  AstChunk(const AstChunk&) = delete;
  AstChunk(AstChunk&&) noexcept = default;
  ~AstChunk() = default;
  AstChunk& operator=(const AstChunk&) = delete;
  AstChunk& operator=(AstChunk&&) = default;

  template<typename T> T* makeNode();

public:
  Root* root = nullptr;
  std::unordered_map<std::string, Type*> referencedTypes;
  std::vector<Type*> createdTypes;
  std::vector<Type*> importedTypes;

private:
  #define FOR_EACH_TAGGED_UNION_TYPE(XX) \
    XX(root, Root, Root) \
    XX(funcList, FuncList, FuncList) \
    XX(func, Func, Func) \
    XX(block, Block, Block) \
    XX(statement, Statement, Statement) \
    XX(variableDeclaration, VariableDeclaration, VariableDeclaration) \
    XX(assignment, Assignment, Assignment) \
    XX(returnStatement, ReturnStatement, ReturnStatement) \
    XX(type, Type, Type) \
    XX(expression, Expression, Expression) \
    XX(op, Op, Op) \
    XX(classN, Class, Class) \
    XX(ifElseChain, IfElseChain, IfElseChain) \
    XX(ifElseChainItem, IfElseChainItem, IfElseChainItem) \
    XX(scope, Scope, Scope)
  #define CLASS_NAME Node
  #include "CreateTaggedUnion.hpp"

  using NodeBlock = std::vector<Node>;
  std::vector<NodeBlock> nodeBlocks;
};

template<typename T> T* AstChunk::makeNode()
{
  constexpr size_t blockSize = 256;
  if (nodeBlocks.empty() || nodeBlocks.back().size() == blockSize)
  {
    nodeBlocks.resize(nodeBlocks.size() + 1);
    nodeBlocks.back().reserve(blockSize);
  }

  NodeBlock& currentBlock = nodeBlocks.back();
  Node& node = currentBlock.emplace_back(T());
  return &node.get<T>();
}
