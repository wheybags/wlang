#pragma once
#include <memory>
#include "AstChunk.hpp"

class MergedAst
{
public:
 AstChunk* create(const std::string& path);

 void link(AstChunk* newChunk);

private:
  std::unordered_map<std::string, std::unique_ptr<AstChunk>> chunks;

public:
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
};