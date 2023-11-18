#include "catui_server.h"
#include "catui.h"
#include "msgstream.h"

#include <unixsocket.h>

#include <cjson/cJSON.h>
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

msgstream_size catui_server_encode_ack(void *buf, size_t buf_size, FILE *err) {
  return 0;
}

msgstream_size catui_server_encode_nack(void *buf, size_t buf_size,
                                        char *err_to_send, FILE *err) {
  cJSON *obj = cJSON_CreateObject();
  if (!obj) {
    if (err)
      fprintf(err, "Failed to create nack json object\n");
    return MSGSTREAM_ERR;
  }

  cJSON *err_str = cJSON_AddStringToObject(obj, "error", err_to_send);
  if (!err_str) {
    if (err)
      fprintf(err,
              "Failed to create json string for 'error' property on nack with "
              "message '%s'\n",
              err_to_send);
    return MSGSTREAM_ERR;
  }

  if (!cJSON_PrintPreallocated(obj, buf, buf_size, 0)) {
    if (err)
      fprintf(
          err,
          "Failed to encode nack with message '%s' in buffer of size '%lu'\n",
          err_to_send, buf_size);
    return MSGSTREAM_ERR;
  }

  cJSON_Delete(obj);
  return strlen(buf);
}

int catui_server_ack(int fd, FILE *err) {
  char ack[CATUI_ACK_SIZE];
  msgstream_size n = catui_server_encode_ack(ack, sizeof(ack), err);

  if (n < 0)
    return -1;

  if (msgstream_send(fd, ack, CATUI_ACK_SIZE, n, err) < 0) {
    fprintf(err, "Failed to send catui ack\n");
    return -1;
  }

  return 0;
}

int catui_server_nack(int fd, char *err_to_send, FILE *err) {
  char err_json[CATUI_ACK_SIZE];
  msgstream_size n =
      catui_server_encode_nack(err_json, sizeof(err_json), err_to_send, err);

  if (n < 0)
    return -1;

  if (msgstream_send(fd, err_json, sizeof(err_json), n, err) < 0) {
    fprintf(err, "Failed to send catui nack\n");
    return -1;
  }

  return 0;
}
