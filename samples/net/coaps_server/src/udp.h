/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UDP_H_
#define _UDP_H_

#include <net/net_core.h>

struct udp_context {
	struct net_context *net_ctx;
	struct net_buf *rx_nbuf;
	struct k_sem rx_sem;
	int remaining;
	char client_id;
};

int udp_init(struct udp_context *ctx);
int udp_tx(void *ctx, const unsigned char *buf, size_t size);
int udp_rx(void *ctx, unsigned char *buf, size_t size);

#endif
