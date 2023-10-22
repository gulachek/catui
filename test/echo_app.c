#include "catui.h"
#include <stdio.h>

int main(int argc, char **argv) {
  int fd = catui_connect("com.example.echo", "1.0.0", stderr);
  if (fd == -1) {
    return 1;
  }

  FILE *echo = fdopen(fd, "r+");
  if (!echo) {
    perror("fdopen");
    return 1;
  }

  fprintf(echo, "Hello world!\n");

  char buf[128];
  fscanf(echo, "%s", buf);

  printf("Received: %s\n", buf);
  fclose(echo);

  return 0;
}
