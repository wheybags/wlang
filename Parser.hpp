#pragma once
#include <vector>
#include <string>
#include "Ast.hpp"

class ParseTree
{
public:
  const Root* parse(const std::vector<std::string>& tokenStrings);
  template<typename T> T* makeNode();

private:
  using NodeBlock = std::vector<Node>;
  std::vector<NodeBlock> nodeBlocks;
};




