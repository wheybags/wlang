#include "MergedAst.hpp"

AstChunk* MergedAst::create(const std::string& path)
{
  auto it = this->chunks.find(path);
  release_assert(it == this->chunks.end());
  return this->chunks.emplace_hint(it, path, new AstChunk())->second.get();
}

void MergedAst::link(AstChunk* newChunk)
{

}
