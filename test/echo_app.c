#include "catui.h"

#include <msgstream.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int fd = catui_connect("com.example.echo", "1.0.0", stderr);
  if (fd == -1) {
    fprintf(stderr, "Failed to connect to catui\n");
    return 1;
  }

  char buf[1025];
  size_t bufsz = sizeof(buf) - 1; // for null terminator
  buf[bufsz] = 0;

  while (fgets(buf, bufsz, stdin)) {
    int ec;

    if ((ec = msgstream_fd_send(fd, buf, bufsz, strlen(buf)))) {
      fprintf(stderr, "msgstream_fd_send: %s\n", msgstream_errstr(ec));
      return 1;
    }

    size_t msg_size;
    if ((ec = msgstream_fd_recv(fd, buf, bufsz, &msg_size))) {
      fprintf(stderr, "msgstream_fd_recv: %s\n", msgstream_errstr(ec));
      return 1;
    }

    printf("Read message: %s", buf);
  }

  return 0;
}
