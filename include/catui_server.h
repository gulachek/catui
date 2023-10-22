#ifndef CATUI_SERVER_H
#define CATUI_SERVER_H

#include <stddef.h>
#include <stdio.h>

int catui_server_fd(FILE *err);
int catui_server_accept(int fd, FILE *err);

int catui_server_ack(int fd, FILE *err);
int catui_server_nack(int fd, char *err_to_send, FILE *err);

#endif
