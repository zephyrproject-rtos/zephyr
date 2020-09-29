/*
 * Copyright (c) 2017 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Value of 0 will cause the IP stack to select next free port */
#define MY_PORT 0

#define PEER_PORT 4242

#if defined(CONFIG_USERSPACE)
#include <app_memory/app_memdomain.h>
extern struct k_mem_partition app_partition;
extern struct k_mem_domain app_domain;
#define APP_BMEM K_APP_BMEM(app_partition)
#define APP_DMEM K_APP_DMEM(app_partition)
#else
#define APP_BMEM
#define APP_DMEM
#endif

struct data {
	const char *proto;

	struct {
		int sock;
		/* Work controlling udp data sending */
		struct k_delayed_work recv;
		struct k_delayed_work transmit;
		uint32_t expecting;
		uint32_t counter;
		uint32_t mtu;
	} udp;

	struct {
		int sock;
		uint32_t expecting;
		uint32_t received;
		uint32_t counter;
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
