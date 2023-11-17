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
  using Node = std::variant<
    std::monostate,
# define X(Type) Type,
    FOR_EACH_AST_TYPE
# undef X
    Scope>;

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
  return &std::get<T>(node);
}
