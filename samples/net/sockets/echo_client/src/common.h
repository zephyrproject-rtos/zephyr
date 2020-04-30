/*
 * Copyright (c) 2017 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Value of 0 will cause the IP stack to select next free port */
#ifndef CONFIG_NET_CONFIG_MY_PORT
#define CONFIG_NET_CONFIG_MY_PORT 0
#endif

#ifndef CONFIG_NET_CONFIG_PEER_PORT
#define CONFIG_NET_CONFIG_PEER_PORT 4242
#endif

struct data {
	const char *proto;

	struct {
		int sock;
		/* Work controlling udp data sending */
		struct k_delayed_work recv;
		struct k_delayed_work transmit;
		u32_t expecting;
		u32_t counter;
		u32_t mtu;
	} udp;

	struct {
		int sock;
		u32_t expecting;
		u32_t received;
		u32_t counter;
	} tcp;
};

struct configs {
	struct data ipv4;
	struct data ipv6;
};

#if !defined(CONFIG_NET_CONFIG_PEER_IPV4_ADDR)
#define CONFIG_NET_CONFIG_PEER_IPV4_ADDR ""
#endif

#if !defined(CONFIG_NET_CONFIG_PEER_IPV6_ADDR)
#define CONFIG_NET_CONFIG_PEER_IPV6_ADDR ""
#endif

extern const char lorem_ipsum[];
extern const int ipsum_len;
extern struct configs conf;

int start_udp(void);
int process_udp(void);
void stop_udp(void);

int start_tcp(void);
int process_tcp(void);
void stop_tcp(void);

#if defined(CONFIG_NET_VLAN)
int init_vlan(void);
#else
static inline int init_vlan(void)
{
	return 0;
}
#endif
