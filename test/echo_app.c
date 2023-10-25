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
    if (msgstream_send(fd, buf, bufsz, strlen(buf), stderr) == -1)
      return 1;

    if (msgstream_recv(fd, buf, bufsz, stderr) < 0) {
      return 1;
    }

    printf("Read message: %s", buf);
  }

  return 0;
}
