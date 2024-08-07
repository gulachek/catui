#include "catui.h"
#include "unixsocket.h"
#include <msgstream.h>

#include <assert.h>
#include <cjson/cJSON.h>
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

  char buf[CATUI_CONNECT_SIZE];

  ssize_t n = snprintf(
      buf, sizeof(buf),
      "{\"catui-version\":\"0.1.0\",\"protocol\":\"%s\",\"version\":\"%s\"}",
      proto, semver);

  if (msgstream_fd_send(sock, buf, sizeof(buf), n)) {
    fprintf(err, "Failed to send handshake request\n");
    return -1;
  }

  size_t msg_size;
  int ec = msgstream_fd_recv(sock, buf, sizeof(buf), &msg_size);
  if (ec) {
    fprintf(err, "Failed to read ack response: %s\n", msgstream_errstr(ec));
    return -1;
  }

  // zero length error message means success
  if (msg_size != 0) {
    fprintf(err, "Received a nack response from server\n");
    return -1;
  }

  return sock;
}

typedef char semver_buf[CATUI_VERSION_SIZE];
static int semver_encode(const catui_semver *v, char *buf) {
  int n = snprintf(buf, CATUI_VERSION_SIZE, "%u.%u.%u", v->major, v->minor,
                   v->patch);

  return 0 <= n && n < CATUI_VERSION_SIZE;
}

int catui_encode_connect(const catui_connect_request *req, void *buf,
                         size_t bufsz, size_t *msgsz) {
  cJSON *obj = cJSON_CreateObject();
  if (!obj)
    return 0;

  semver_buf catuiv, protov;

  if (!semver_encode(&req->catui_version, catuiv))
    return 0;

  if (!semver_encode(&req->version, protov))
    return 0;

  cJSON *jcatuiv = cJSON_CreateString(catuiv);
  if (!jcatuiv)
    return 0;

  cJSON *jproto = cJSON_CreateString(req->protocol);
  if (!jproto)
    return 0;

  cJSON *jprotov = cJSON_CreateString(protov);
  if (!jprotov)
    return 0;

  if (!cJSON_AddItemToObject(obj, "catui-version", jcatuiv))
    return 0;

  if (!cJSON_AddItemToObject(obj, "protocol", jproto))
    return 0;

  if (!cJSON_AddItemToObject(obj, "version", jprotov))
    return 0;

  if (!cJSON_PrintPreallocated(obj, buf, bufsz, 0))
    return 0;

  cJSON_free(obj);

  *msgsz = strlen(buf);
  return 1;
}
