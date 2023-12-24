#pragma once
#if defined(__APPLE__) || defined(linux)
#include <cstdio>

int w_close(int fd);
ssize_t w_read(int fd, void* buf, size_t size);
ssize_t	w_write(int fd, const void* buf, size_t size);
int w_dup2(int oldfd, int newfd);

#endif