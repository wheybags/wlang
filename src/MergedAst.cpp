#include "MergedAst.hpp"
#include "BuiltinTypes.hpp"
#include "AstChunk.hpp"

MergedAst::MergedAst()
{
  for (const auto& pair : BuiltinTypes::inst.typeMap)
    this->linkScope.types.insert_or_assign(pair.first, Scope::Item<Type*>{.item = pair.second, .chunk = nullptr});
}

AstChunk* MergedAst::create(std::string_view path)
{
  auto it = this->chunks.find(path);
  release_assert(it == this->chunks.end());
  return this->chunks.emplace_hint(it, path, new AstChunk())->second.get();
}

void MergedAst::link(AstChunk* chunk)
{
  Scope* chunkScope = chunk->root->funcList->scope;
  release_assert(!chunkScope->parent);

  for (auto& item : chunkScope->types)
    this->linkScope.types.insert(item);
  chunkScope->types.clear();

  for (const auto& item : chunkScope->variables)
    this->linkScope.variables.insert(item);
  chunkScope->variables.clear();

  for (const auto& item : chunkScope->functions)
    this->linkScope.functions.insert(item);
  chunkScope->functions.clear();

  for (Func* function : chunk->root->funcList->functions)
  {
    std::string name = function->name;

    if (function->memberClass)
      name = function->memberClass->type->name + "_" + function->name;

    release_assert(!this->usedMangledNames.contains(name));

    this->usedMangledNames.insert(name);
    function->mangledName = std::move(name);
  }

  chunkScope->parent = &this->linkScope;
}

template<typename T>
static void removeChunkPart(AstChunk* chunk, HashMap<Scope::Item<T*>> map)
{
  for (auto it = map.begin(); it != map.end();)
  {
    if (it->second.chunk == chunk)
      it = map.erase(it);
    else
      ++it;
  }
}

void MergedAst::tryRemoveChunk(std::string_view path)
{
  auto it = this->chunks.find(path);
  if (it == this->chunks.end())
    return;

  AstChunk* chunk = it->second.get();

  removeChunkPart(chunk, this->linkScope.types);
  removeChunkPart(chunk, this->linkScope.variables);
  removeChunkPart(chunk, this->linkScope.variables);

  for (Func* function : chunk->root->funcList->functions)
    this->usedMangledNames.erase(function->mangledName);

  this->chunks.erase(it);
}

