#pragma once
#include <cstdint>
#include <atomic>

class GenerationPtrBase
{
  static void newGeneration() { thisThreadGeneration = nextGeneration++; }

protected:
  static thread_local uint64_t thisThreadGeneration;
  static std::atomic_uint64_t nextGeneration;
};

template<typename T>
class GenerationPtr : GenerationPtrBase
{
public:
  GenerationPtr() : GenerationPtr(nullptr) {}
  GenerationPtr(T* ptr) : ptr(ptr), ptrGeneration(thisThreadGeneration) {}
  GenerationPtr(const GenerationPtr& other) = default;
  GenerationPtr& operator=(const GenerationPtr&) = default;

  T* get() { return thisThreadGeneration == ptrGeneration ? ptr : nullptr; }
  const T* get() const { return thisThreadGeneration == ptrGeneration ? ptr : nullptr; }
  T* operator*() { return thisThreadGeneration == ptrGeneration ? ptr : nullptr; }
  const T* operator*() const { return thisThreadGeneration == ptrGeneration ? ptr : nullptr; }
  T* operator->() { return thisThreadGeneration == ptrGeneration ? ptr : nullptr; }
  const T* operator->() const { return thisThreadGeneration == ptrGeneration ? ptr : nullptr; }

  operator bool() const { return get(); }
  bool operator==(GenerationPtr other) const{ return get() == other.get(); }

private:
  T* ptr = nullptr;
  uint64_t ptrGeneration = 0;
};