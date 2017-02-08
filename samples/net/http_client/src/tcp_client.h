/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_

#include <net/net_context.h>
#include <net/net_ip.h>

struct tcp_client_ctx {
	/* IP stack network context */
	struct net_context *net_ctx;
	/* Local sock address */
	struct sockaddr local_sock;
	/* Network timeout */
	int32_t timeout;
	/* User defined call back*/
	void (*receive_cb)(struct tcp_client_ctx *ctx, struct net_buf *rx);
};

int tcp_set_local_addr(struct tcp_client_ctx *ctx, const char *local_addr);

int tcp_connect(struct tcp_client_ctx *ctx, const char *server_addr,
		uint16_t server_port);

int tcp_disconnect(struct tcp_client_ctx *ctx);

#endif
