/*
 * Copyright (c) 2020 InnBlue
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file tftp.h
 *
 *  @brief Zephyr TFTP Implementation
 */

#ifndef ZEPHYR_INCLUDE_NET_TFTP_H_
#define ZEPHYR_INCLUDE_NET_TFTP_H_

#include <zephyr/zephyr.h>
#include <zephyr/net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tftpc {
	uint8_t   *user_buf;
	uint32_t  user_buf_size;
};

/* TFTP Client Error codes. */
#define TFTPC_SUCCESS             0
#define TFTPC_DUPLICATE_DATA     -1
#define TFTPC_BUFFER_OVERFLOW    -2
#define TFTPC_UNKNOWN_FAILURE    -3
#define TFTPC_REMOTE_ERROR       -4
#define TFTPC_RETRIES_EXHAUSTED  -5

/* @brief This function gets "file" from the remote server.
 *
 * If the file is successfully received its size will be returned in
 * `client->user_buf_size` parameter.
 *
 * @param server      Control Block that represents the remote server.
 * @param client      Client Buffer Information.
 * @param remote_file Name of the remote file to get.
 * @param mode        TFTP Client "mode" setting
 *
 * @return TFTPC_SUCCESS if the operation completed successfully.
 *         TFTPC_BUFFER_OVERFLOW if the file is larger than the user buffer.
 *         TFTPC_REMOTE_ERROR if the server failed to process our request.
 *         TFTPC_RETRIES_EXHAUSTED if the client timed out waiting for server.
 */
int tftp_get(struct sockaddr *server, struct tftpc *client,
	     const char *remote_file, const char *mode);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_TFTP_H_ */
