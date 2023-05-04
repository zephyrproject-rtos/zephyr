/*
 * Copyright (c) 2017 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/* Value of 0 will cause the IP stack to select next free port */
#define MY_PORT 0

#define PEER_PORT 4242

#if defined(CONFIG_USERSPACE)
#include <zephyr/app_memory/app_memdomain.h>
extern struct k_mem_partition app_partition;
extern struct k_mem_domain app_domain;
#define APP_BMEM K_APP_BMEM(app_partition)
#define APP_DMEM K_APP_DMEM(app_partition)
#else
#define APP_BMEM
#define APP_DMEM
#endif

#if defined(CONFIG_NET_TC_THREAD_PREEMPTIVE)
#define THREAD_PRIORITY K_PRIO_PREEMPT(8)
#else
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#endif

#define UDP_STACK_SIZE 2048

struct udp_control {
	struct k_poll_signal tx_signal;
	struct k_timer tx_timer;
	struct k_timer rx_timer;
};

struct data {
	const char *proto;

	struct {
		int sock;
		uint32_t expecting;
		uint32_t counter;
		uint32_t mtu;
		struct udp_control *ctrl;
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

/* init_udp initializes kernel objects, hence it has to be called from
 * supervisor thread.
 */
void init_udp(void);
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
