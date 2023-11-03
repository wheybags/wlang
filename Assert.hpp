#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

#ifdef _MSC_VER
#define DEBUG_BREAK __debugbreak()
#else
#include <csignal>
#define DEBUG_BREAK raise(SIGTRAP);
#endif

[[noreturn]] inline void message_and_abort_fmt(const char* message, ...)
{
  va_list args;
  va_start(args, message);
  vfprintf(stderr, message, args);
  va_end(args);

  DEBUG_BREAK;
  abort();
}

#define message_and_abort(message) message_and_abort_fmt("%s\n", message)

inline void real_doAssert(bool condition, const char* message, const char* file, int line)
{
  if (!condition)
    message_and_abort_fmt("ASSERTION FAILED: (%s) in %s:%d\n", message, file, line);
}

#define release_assert(cond) real_doAssert(cond, #cond, __FILE__, __LINE__)

#ifdef NDEBUG
#define debug_assert(cond) do {} while(0)
#else
#define debug_assert(cond) release_assert(cond)
#endif