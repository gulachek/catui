#include "catui.h"
#include "unixsocket.h"
#include <msgstream.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <unistd.h>

static void fperror(FILE *err, const char *s) {
  if (!err)
    return;
  char *msg = strerror(errno);
  if (s)
    fprintf(err, "%s: %s\n", s, msg);
  else
    fprintf(err, "%s\n", msg);
}

static const char *catui_address() {
  const char *address = getenv("CATUI_ADDRESS");

  if (address)
    return address;

  // hard coded for mac
  return "~/Library/Caches/TemporaryItems/catui/load_balancer.sock";
}

int catui_connect(const char *proto, const char *semver, FILE *err) {
  const char *addr = catui_address();

  int sock = unix_socket(SOCK_STREAM);
  if (sock == -1) {
    fprintf(err, "Failed to allocate unix_socket\n");
    return -1;
  }

  if (unix_connect(sock, addr) == -1) {
    fprintf(err, "Failed to connect to %s\n", addr);
    return -1;
  }

  char buf[1024];

  ssize_t n = snprintf(
      buf, sizeof(buf),
      "{\"catui-version\":\"0.1.0\",\"protocol\":\"%s\",\"version\":\"%s\"}",
      proto, semver);

  msgstream_size msg_size = msgstream_send(sock, buf, sizeof(buf), n, err);
  if (msg_size < 0) {
    fprintf(err, "Failed to send handshake request\n");
    return -1;
  }

  msg_size = msgstream_recv(sock, buf, sizeof(buf), err);
  if (msg_size < 0) {
    if (msg_size == MSGSTREAM_EOF) {
      fprintf(err, "Encountered EOF\n");
    }

    fprintf(err, "Failed to read ack response\n");
    return -1;
  }

  // zero length error message means success
  if (msg_size != 0) {
    fprintf(err, "Received a nack response from server\n");
    return -1;
  }

  return sock;
}
