#include "catui_server.h"
#include <msgstream.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/select.h>
#include <unistd.h>

#define NCLIENTS 128
struct client_entry {
  int is_active;
  int fd;
};

int main(int argc, char **argv) {
  fprintf(stderr, "Starting echo_server...\n");

  int lb = catui_server_fd(stderr);
  if (lb == -1) {
    fprintf(stderr, "Failed to acquire load balancer connection\n");
    return 1;
  }

  fprintf(stderr, "Load balancer at descriptor %d\n", lb);

  struct client_entry clients[NCLIENTS] = {};

  while (1) {
    fd_set readset;
    FD_ZERO(&readset);

    int max_fd = lb;
    FD_SET(lb, &readset);

    for (int i = 0; i < NCLIENTS; ++i) {
      struct client_entry *client = &clients[i];
      if (!client->is_active)
        continue;

      FD_SET(client->fd, &readset);
      max_fd = MAX(max_fd, client->fd);
    }

    if (select(max_fd + 1, &readset, NULL, NULL, NULL) == -1) {
      perror("select");
      return 1;
    }

    if (FD_ISSET(lb, &readset)) {
      int con = catui_server_accept(lb, stderr);
      if (con == -1) {
        return -1;
      }

      struct client_entry *connected_client = NULL;

      for (int i = 0; i < NCLIENTS; ++i) {
        struct client_entry *client = &clients[i];
        if (!client->is_active) {
          connected_client = client;
          break;
        }
      }

      if (connected_client) {
        if (catui_server_ack(con, stderr) != -1) {
          connected_client->is_active = 1;
          connected_client->fd = con;
        }
      } else {
        catui_server_nack(con, "Max connections", stderr);
      }
    }

    for (int i = 0; i < NCLIENTS; ++i) {
      struct client_entry *client = &clients[i];
      if (!(client->is_active && FD_ISSET(client->fd, &readset)))
        continue;

      char buf[1024];
      int con = client->fd;

      msgstream_size nread = msgstream_recv(con, buf, sizeof(buf), stderr);

      if (nread >= 0) {
        nread = msgstream_send(con, buf, sizeof(buf), nread, stderr);
      }

      if (nread < 0) {
        close(con);
        memset(client, 0, sizeof(struct client_entry));
      }
    }
  }

  return 0;
}
