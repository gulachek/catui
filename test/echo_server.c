#include "buffer.h"
#include "catui_server.h"
#include <stdio.h>

int main(int argc, char **argv) {
  char err_bytes[1024];
  buffer err = MAKEBUFA(err_bytes);

  int lb = catui_server_fd(&err);
  if (lb == -1) {
    fprintf(stderr, "%s\n", err_bytes);
    return 1;
  }

  int con = catui_server_accept(lb, &err);
  if (con == -1) {
    fprintf(stderr, "%s\n", err_bytes);
    return -1;
  }

  if (catui_server_ack(con, &err) == -1) {
    fprintf(stderr, "%s\n", err_bytes);
    return -1;
  }

  FILE *pCon = fdopen(con, "r+");
  if (!pCon) {
    fprintf(stderr, "Failed to open stream for connection\n");
    return -1;
  }

  char buf[512];
  if (!fgets(buf, sizeof(buf), pCon)) {
    fclose(pCon);
    return 1;
  }

  fputs(buf, pCon);
  fclose(pCon);
  return 0;
}
