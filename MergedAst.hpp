#pragma once
#include <memory>
#include "AstChunk.hpp"

class MergedAst
{
public:
  MergedAst();
  MergedAst(const MergedAst&) = delete;
  MergedAst(MergedAst&&) = delete;
  MergedAst& operator=(const MergedAst&) = delete;
  MergedAst& operator=(MergedAst&&) = delete;

  AstChunk* create(std::string_view path);
  void link(AstChunk* chunk);
  void tryRemoveChunk(std::string_view path);

  struct iterator
  {
    AstChunk& operator->() { return *realIt->second; }
    AstChunk* operator*() { return realIt->second.get(); }
    iterator& operator++() { realIt++; return *this; }
    iterator operator++(int) { iterator tmp = *this; ++realIt; return tmp; }
    bool operator==(const iterator& other) const { return realIt == other.realIt; }
    bool operator!=(const iterator& other) const { return realIt != other.realIt; }
  private:
    std::unordered_map<std::string, std::unique_ptr<AstChunk>>::iterator realIt;
    friend class MergedAst;
  };

  iterator begin() { iterator it; it.realIt = chunks.begin(); return it; }
  iterator end() { iterator it; it.realIt = chunks.end(); return it; }

private:
  HashMap<std::unique_ptr<AstChunk>> chunks;
  Scope linkScope;
};