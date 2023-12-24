#if defined(__APPLE__) || defined(linux)
#include "UnixWrap.hpp"
#include <unistd.h>
#include <cerrno>

#ifdef __APPLE__
  // normal close on osx leaves fd in indeterminate state if interrupted (return -1, set errno to EINTR).
  // linux guarantees that fds are closed when close returns, osx can too if you use close$NOCANCEL
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"
  extern "C" int close$NOCANCEL(int);
  #define close_no_cancel close$NOCANCEL
  #pragma clang diagnostic pop
#endif

#ifdef linux
#define close_no_cancel close
#endif

int w_close(int fd)
{
  return close_no_cancel(fd);
}

ssize_t w_read(int fd, void* buf, size_t size)
{
  while (true)
  {
    ssize_t ret = read(fd, buf, size);
    if (ret != -1 || errno != EINTR)
      return ret;
  }
}

ssize_t	w_write(int fd, const void* buf, size_t size)
{
  while (true)
  {
    ssize_t ret = write(fd, buf, size);
    if (ret != -1 || errno != EINTR)
      return ret;
  }
}

int w_dup2(int oldfd, int newfd)
{
  while (true)
  {
    int ret = dup2(oldfd, newfd);
    if (ret != -1 || errno != EINTR)
      return ret;
  }
}

#endif