/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SSL_UTILS_H_
#define _SSL_UTILS_H_

#include <net/net_core.h>
#include <net/http_parser.h>

struct rx_fifo_block {
	sys_snode_t snode;
	struct k_mem_block block;
	struct net_buf *buf;
};

struct ssl_context {
	struct net_context *net_ctx;
	struct net_buf *rx_nbuf;
	struct net_buf *frag;
	struct k_sem tx_sem;
	struct k_fifo rx_fifo;
	struct http_parser_settings parser_settings;
	struct http_parser parser;

	int remaining;
};

int ssl_init(struct ssl_context *ctx, void *addr);
int ssl_tx(void *ctx, const unsigned char *buf, size_t size);
int ssl_rx(void *ctx, unsigned char *buf, size_t size);

void https_server_start(void);

#endif
