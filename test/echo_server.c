#include "catui_server.h"
#include <msgstream.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  fprintf(stderr, "Starting echo_server...\n");

  int lb = catui_server_fd(stderr);
  if (lb == -1) {
    fprintf(stderr, "Failed to acquire load balancer connection\n");
    return 1;
  }

  fprintf(stderr, "Load balancer at descriptor %d\n", lb);

  while (1) {
    int con = catui_server_accept(lb, stderr);
    if (con == -1) {
      return -1;
    }

    fprintf(stderr, "Accepted connection at descriptor %d\n", con);

    if (catui_server_ack(con, stderr) == -1) {
      return -1;
    }

    fprintf(stderr, "Sent ack\n");

    char buf[1024];
    msgstream_size nread = 0;
    while ((nread = msgstream_recv(con, buf, sizeof(buf), stderr)) >= 0) {
      msgstream_send(con, buf, sizeof(buf), nread, stderr);
    }

    if (nread == MSGSTREAM_EOF)
      continue;
    else
      return 1;
  }

  return 0;
}
