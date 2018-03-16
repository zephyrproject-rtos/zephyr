/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <net/net_if.h>
#include <net/net_app.h>

#define MY_PORT 4242

#define MAX_DBG_PRINT 64

struct interfaces {
	struct net_if *non_vlan;
	struct net_if *first_vlan;
	struct net_if *second_vlan;
};

void start_udp(void);
void stop_udp(void);

void start_tcp(void);
void stop_tcp(void);

struct net_pkt *build_reply_pkt(const char *name,
				struct net_app_ctx *ctx,
				struct net_pkt *pkt);
void pkt_sent(struct net_app_ctx *ctx, int status,
	      void *token, void *user_data);
void panic(const char *msg);
void quit(void);

int setup_vlan(struct interfaces *interfaces);
void setup_echo_server(void);
void cleanup_echo_server(void);
