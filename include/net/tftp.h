/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file tftp.h
 *
 */

#ifndef ZEPHYR_INCLUDE_NET_TFTP_H_
#define ZEPHYR_INCLUDE_NET_TFTP_H_

#include <zephyr.h>
#include <net/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tftpc {
	u8_t   *user_buf;
	u32_t  user_buf_size;
};

/* Name: tftp_get
 * Description: This function gets "file" from the remote server.
 */
int tftp_get(struct sockaddr_in *server, struct tftpc *client,
		     const char *remote_file, const char *mode);

/* Name: tftp_get
 * Description: This function puts "file" to the remote server.
 */
int tftp_put(struct sockaddr_in *server, struct tftpc *client,
		     const char *remote_file, const char *mode);

#endif /* ZEPHYR_INCLUDE_NET_TFTP_H_ */

