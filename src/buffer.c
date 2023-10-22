#include "buffer.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <sys/errno.h>

int buf_printf(buffer *buf, const char *fmt, ...) {
  if (!buf)
    return -1;

  va_list args;
  va_start(args, fmt);
  int ret = vsnprintf(buf->data, buf->size, fmt, args);
  va_end(args);
  return ret;
}

int buf_perror(buffer *buf) {
  if (!buf)
    return -1;

  return strerror_r(errno, buf->data, buf->size);
}
