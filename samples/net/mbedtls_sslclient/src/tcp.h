/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TCP_H_
#define _TCP_H_

#include <sys/types.h>
#include <net/socket.h>

struct tcp_context {
	int socket;
	struct sockaddr local_sock;
	s32_t timeout;
};

int tcp_init(struct tcp_context *ctx, const char *server_addr,
		u16_t server_port);

int tcp_tx(void *ctx, const unsigned char *buf, size_t size);
int tcp_rx(void *ctx, unsigned char *buf, size_t size);
int tcp_set_local_addr(struct tcp_context *ctx, const char *local_addr);

#endif
