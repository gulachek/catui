#ifndef CATUI_SERVER_H
#define CATUI_SERVER_H

#include "catui.h" // for consumer to have CATUI_ACK_SIZE

#include <stddef.h>
#include <stdio.h>

int catui_server_fd(FILE *err);
int catui_server_accept(int fd, FILE *err);

/**
 * Encode an ack response to be sent as a msgstream message
 *
 * @param buf Buffer to hold bytes
 * @param buf_size size of allocated buffer 'buf'
 * @param err Optional stream for error messages to be written to
 * @returns size of message if successful, < 0 on error
 */
int16_t catui_server_encode_ack(void *buf, size_t buf_size, FILE *err);

/**
 * Encode a nack response to be sent as a msgstream message
 *
 * @param buf Buffer to hold bytes
 * @param buf_size size of allocated buffer 'buf'
 * @param err_to_send null terminated string to be sent as an error message
 * @param err Optional stream for error messages to be written to
 * @returns size of message if successful, < 0 on error
 */
int16_t catui_server_encode_nack(void *buf, size_t buf_size, char *err_to_send,
                                 FILE *err);

int16_t catui_server_ack(int fd, FILE *err);
int16_t catui_server_nack(int fd, char *err_to_send, FILE *err);

#endif
