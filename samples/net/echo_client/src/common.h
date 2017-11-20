/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Value of 0 will cause the IP stack to select next free port */
#define MY_PORT 0

#define PEER_PORT 4242

#define WAIT_TIME  K_SECONDS(10)
#define CONNECT_TIME  K_SECONDS(10)

struct data {
	/* Work controlling udp data sending */
	struct k_delayed_work recv;
	struct net_app_ctx *udp;

	const char *proto;
	u32_t expecting_udp;
	u32_t expecting_tcp;
	u32_t received_tcp;
};

struct configs {
	struct data ipv4;
	struct data ipv6;
};

#if !defined(CONFIG_NET_APP_PEER_IPV4_ADDR)
#define CONFIG_NET_APP_PEER_IPV4_ADDR ""
#endif

#if !defined(CONFIG_NET_APP_PEER_IPV6_ADDR)
#define CONFIG_NET_APP_PEER_IPV6_ADDR ""
#endif

extern const char lorem_ipsum[];
extern int ipsum_len;
extern struct configs conf;
extern struct k_sem tcp_ready;

void start_udp(void);
void stop_udp(void);

int start_tcp(void);
void stop_tcp(void);

struct net_pkt *prepare_send_pkt(struct net_app_ctx *ctx,
				 const char *name,
				 int *expecting_len);
void panic(const char *msg);
