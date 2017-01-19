/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TCP_H_
#define _TCP_H_

#include <net/net_core.h>
#include <net/buf.h>

struct tcp_context {
	struct net_context *net_ctx;
	struct net_buf *rx_nbuf;
	int remaining;
};

int tcp_init(struct tcp_context *ctx);
int tcp_tx(void *ctx, const unsigned char *buf, size_t size);
int tcp_rx(void *ctx, unsigned char *buf, size_t size);

#endif
