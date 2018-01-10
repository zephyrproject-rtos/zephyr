/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define MY_PORT 42042

#define MAX_DBG_PRINT 64

struct tp_stats {
	struct {
		u32_t prev_recv;
		u32_t recv;
		u32_t sent;
		u32_t dropped;
	} pkts;

	struct {
		u32_t recv;
		u32_t sent;
	} bytes;
};

extern struct tp_stats tp_stats;

void print_statistics(void);

void start_udp(void);
void stop_udp(void);

struct net_pkt *build_reply_pkt(const char *name,
				struct net_app_ctx *ctx,
				struct net_pkt *pkt);
void pkt_sent(struct net_app_ctx *ctx, int status,
	      void *token, void *user_data);
