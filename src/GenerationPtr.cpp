#include "GenerationPtr.hpp"

thread_local uint64_t GenerationPtrBase::thisThreadGeneration = 1;
std::atomic_uint64_t GenerationPtrBase::nextGeneration = 1;