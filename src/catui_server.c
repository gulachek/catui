#include "catui_server.h"
#include "msgstream.h"
#include "unixsocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

void fperror(FILE *err, char *s) {
  if (!err)
    return;
  char *msg = strerror(errno);
  if (s)
    fprintf(err, "%s: %s\n", s, msg);
  else
    fprintf(err, "%s\n", msg);
}

int catui_server_fd(FILE *err) {
  const char *lb = getenv("CATUI_LOAD_BALANCER_FD");
  if (!lb) {
    fprintf(err, "Environment variable CATUI_LOAD_BALANCER_FD not defined");
    return -1;
  }

  int fd = -1;
  if (sscanf(lb, "%d", &fd) != 1) {
    fprintf(err, "Failed to parse '%s' as a decimal file descriptor", lb);
    return -1;
  }

  return fd;
}

int catui_server_accept(int fd, FILE *err) {
  int con = unix_recv_fd(fd);
  if (con == -1) {
    fprintf(err, "Failed to receive a file descriptor");
    return -1;
  }

  return con;
}

int catui_server_ack(int fd, FILE *err) {
  uint8_t zero = 0;
  if (msgstream_send(fd, &zero, 1024, 0, err) == -1) {
    fprintf(err, "Failed to send catui ack\n");
    return -1;
  }

  return 0;
}

int catui_server_nack(int fd, char *err_to_send, FILE *err) {
  char err_json[1024];
  // TODO - escape this string
  int n =
      snprintf(err_json, sizeof(err_json), "{\"error\":\"%s\"}", err_to_send);
  if (n < 0) {
    fperror(err, "catui_server_nack snprintf");
    return -1;
  }

  if (msgstream_send(fd, err_json, sizeof(err_json), n, err) == -1) {
    fprintf(err, "Failed to send catui nack\n");
    return -1;
  }

  return 0;
}
