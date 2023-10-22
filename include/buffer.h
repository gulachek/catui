#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>

typedef struct _buffer {
  void *data;
  size_t size;
} buffer;

int buf_printf(buffer *buf, const char *fmt, ...);
int buf_perror(buffer *buf);

#define MAKEBUFA(ARRAY)                                                        \
  { .data = ARRAY, .size = sizeof(ARRAY) }

#endif
