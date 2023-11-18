#ifndef CATUI_SERVER_H
#define CATUI_SERVER_H

#include "catui.h" // for consumer to have CATUI_ACK_SIZE

#include <msgstream.h> // for msgstream_size
#include <stddef.h>
#include <stdio.h>

int catui_server_fd(FILE *err);
int catui_server_accept(int fd, FILE *err);

msgstream_size catui_server_encode_ack(void *buf, size_t buf_size, FILE *err);
msgstream_size catui_server_encode_nack(void *buf, size_t buf_size,
                                        char *err_to_send, FILE *err);

int catui_server_ack(int fd, FILE *err);
int catui_server_nack(int fd, char *err_to_send, FILE *err);

#endif
