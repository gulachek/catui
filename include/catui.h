/**
 * Copyright 2025 Nicholas Gulachek
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */
#ifndef CATUI_H
#define CATUI_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CATUI_API
#define CATUI_API
#endif

#define CATUI_ACK_SIZE 1024
#define CATUI_CONNECT_SIZE 1024

/**
 * Connect to a catui server with the given protocol and version
 * @param proto The device communication protocol to connect to
 * @param semver The version of the protocol required for communication
 * @param err Pointer to a stream that will have an error written if present
 * @returns A file descriptor of the connection on success, -1 on failure
 */
int CATUI_API catui_connect(const char *proto, const char *semver, FILE *err);

/**
 * Looks up the catui load balancer's file descriptor, if available
 * @param err A stream that will have an error message written if applicable
 * @returns A file descriptor on success, -1 on failure
 */
int CATUI_API catui_server_fd(FILE *err);

/**
 * Accepts a connection forwarded from the catui load balancer
 * @param err A stream that will have an error message written if applicable
 * @returns A file descriptor on success, -1 on failure
 */
int CATUI_API catui_server_accept(int fd, FILE *err);

/**
 * Encode an ack response to be sent as a msgstream message
 *
 * @param buf Buffer to hold bytes
 * @param buf_size size of allocated buffer 'buf'
 * @param err Optional stream for error messages to be written to
 * @returns size of message if successful, < 0 on error
 */
int16_t CATUI_API catui_server_encode_ack(void *buf, size_t buf_size,
                                          FILE *err);

/**
 * Encode a nack response to be sent as a msgstream message
 *
 * @param buf Buffer to hold bytes
 * @param buf_size size of allocated buffer 'buf'
 * @param err_to_send null terminated string to be sent as an error message
 * @param err Optional stream for error messages to be written to
 * @returns size of message if successful, < 0 on error
 */
int16_t CATUI_API catui_server_encode_nack(void *buf, size_t buf_size,
                                           const char *err_to_send, FILE *err);

/**
 * Send an ack message to a file descriptor
 * @param fd The file descriptor to write to
 * @param err A stream that will have an error message written if applicable
 * @returns size of message if successful, < 0 on error
 */
int16_t CATUI_API catui_server_ack(int fd, FILE *err);

/**
 * Send a nack message to a file descriptor
 * @param fd The file descriptor to write to
 * @param err_to_send The error message to include in the nack message
 * @param err A stream that will have an error message written if applicable
 * @returns size of message if successful, < 0 on error
 */
int16_t CATUI_API catui_server_nack(int fd, const char *err_to_send, FILE *err);

#define CATUI_PROTOCOL_SIZE 128
#define CATUI_VERSION_SIZE 23 // 5maj + 5min + 10pat + 2dots + null

/**
 * Semver structure
 */
typedef struct {
  /// The major version
  uint16_t major;
  /// The minor version
  uint16_t minor;
  /// The patch version
  uint32_t patch;
} catui_semver;

/**
 * Serializes a semver to a null terminated ascii C string
 * @param v The version
 * @param buf The preallocated buffer to hold the new C string. When NULL, only
 * compute string size in return value.
 * @param bufsz The size of buf in bytes. N/A when buf is NULL
 * @return The strlen() of the written string or -1 when a null version is given
 * @remarks When the return value is greater than or equal to bufsz, this means
 * the string was truncated, similar to snprintf. It is recommended to reserve
 * at least CATUI_VERSION_SIZE bytes for the operation to always have enough
 * memory.
 */
int catui_semver_to_string(const catui_semver *v, void *buf, size_t bufsz);

/**
 * Test whether a semver version can support another semver version
 * @param api The API semver version in question
 * @param consumer The consumer's semver version needing support
 * @returns 1 if api can support consumer, 0 otherwise
 * @remarks This is equivalent to catui_semver_can_use(consumer, api)
 */
int catui_semver_can_support(const catui_semver *api,
                             const catui_semver *consumer);

/**
 * Test whether a semver version can use another semver version
 * @param consumer The consumer's semver version needing support
 * @param api The API semver version in question
 * @returns 1 if api can support consumer, 0 otherwise
 * @remarks This is equivalent to catui_semver_can_support(api, consumer)
 */
int catui_semver_can_use(const catui_semver *consumer, const catui_semver *api);

/**
 * Structure representing a catui connect request
 */
typedef struct {
  /// The version of the catui protocol required for communication
  catui_semver catui_version;

  /// The protocol of the device to connect to
  char protocol[CATUI_PROTOCOL_SIZE];

  /// The version of the device protocol to connect to
  catui_semver version;
} catui_connect_request;

/**
 * Encode a connect request
 * @param[in] req The connect request to encode
 * @param[in] buf The buffer to encode the message to
 * @param[in] bufsz The size of buf in bytes
 * @param[out] msgsz The size of the encoded message
 * @returns 1 on success, 0 on failure
 */
int CATUI_API catui_encode_connect(const catui_connect_request *req, void *buf,
                                   size_t bufsz, size_t *msgsz);

/**
 * Decode a connect request
 * @param[in] buf The buffer containing the encoded bytes
 * @param[in] msgsz The size of the encoded message
 * @param[out] req The structure to hold the decoded message
 * @returns 1 on success, 0 on failure
 */
int CATUI_API catui_decode_connect(const void *buf, size_t msgsz,
                                   catui_connect_request *req);

#ifdef __cplusplus
}
#endif

#endif
