#include "MergedAst.hpp"
#include "BuiltinTypes.hpp"
#include "AstChunk.hpp"

MergedAst::MergedAst()
{
  for (const auto& pair : BuiltinTypes::inst.typeMap)
    this->linkScope.types.insert_or_assign(pair.first, Scope::Item<Type*>{.item = pair.second, .chunk = nullptr});
}

AstChunk* MergedAst::create(const std::string& path)
{
  auto it = this->chunks.find(path);
  release_assert(it == this->chunks.end());
  return this->chunks.emplace_hint(it, path, new AstChunk())->second.get();
}

void MergedAst::link(AstChunk* chunk)
{
  for (const auto& item : chunk->root->funcList->scope->types)
    this->linkScope.types.insert_or_assign(item.first, item.second);

  for (const auto& item : chunk->root->funcList->scope->variables)
    this->linkScope.variables.insert_or_assign(item.first, item.second);

  for (const auto& item : chunk->root->funcList->scope->functions)
    this->linkScope.functions.insert_or_assign(item.first, item.second);

  release_assert(!chunk->root->funcList->scope->parent);
  chunk->root->funcList->scope->parent = &this->linkScope;
}

template<typename T>
static void remove(AstChunk* chunk, HashMap<Scope::Item<T*>> map)
{
  for (auto it = map.begin(); it != map.end();)
  {
    if (it->second.chunk == chunk)
      it = map.erase(it);
    else
      ++it;
  }
}

void MergedAst::unlink(AstChunk* chunk)
{
  release_assert(chunk->root->funcList->scope->parent == &this->linkScope);
  chunk->root->funcList->scope->parent = nullptr;
  remove(chunk, this->linkScope.types);
  remove(chunk, this->linkScope.variables);
  remove(chunk, this->linkScope.functions);
}

