/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define MY_PORT 4242

#define MAX_DBG_PRINT 64

void start_udp(void);
void stop_udp(void);

void start_tcp(void);
void stop_tcp(void);

struct net_pkt *build_reply_pkt(const char *name,
				struct net_app_ctx *ctx,
				struct net_pkt *pkt,
				u8_t proto_len);
void pkt_sent(struct net_app_ctx *ctx, int status,
	      void *token, void *user_data);
void panic(const char *msg);
void quit(void);
