/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define MAX_DBG_PRINT 64

void start_server(void);
void stop_server(void);

struct net_pkt *build_reply_pkt(const char *name,
				struct net_app_ctx *ctx,
				struct net_pkt *pkt);
void pkt_sent(struct net_app_ctx *ctx, int status,
	      void *token, void *user_data);
void panic(const char *msg);
void quit(void);
