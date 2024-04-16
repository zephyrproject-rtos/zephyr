/*
 * Copyright (c) 2017-2019 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define MY_PORT 4242
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) || defined(CONFIG_NET_TCP) || \
	defined(CONFIG_COVERAGE_GCOV)
#define STACK_SIZE 4096
#else
#define STACK_SIZE 2048
#endif

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(8)
#endif

#define RECV_BUFFER_SIZE 1280
#define STATS_TIMER 60 /* How often to print statistics (in seconds) */

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

struct data {
	const char *proto;

	struct {
		int sock;
		char recv_buffer[RECV_BUFFER_SIZE];
		uint32_t counter;
		atomic_t bytes_received;
		struct k_work_delayable stats_print;
	} udp;

	struct {
		int sock;
		atomic_t bytes_received;
		struct k_work_delayable stats_print;

		struct {
			int sock;
			char recv_buffer[RECV_BUFFER_SIZE];
			uint32_t counter;
		} accepted[CONFIG_NET_SAMPLE_NUM_HANDLERS];
	} tcp;
};

struct configs {
	struct data ipv4;
	struct data ipv6;
};

extern struct configs conf;

void start_udp(void);
void stop_udp(void);

void start_tcp(void);
void stop_tcp(void);

void quit(void);

#if defined(CONFIG_NET_VLAN)
int init_vlan(void);
#else
static inline int init_vlan(void)
{
	return 0;
}
#endif /* CONFIG_NET_VLAN */

#if defined(CONFIG_NET_L2_IPIP)
int init_tunnel(void);
bool is_tunnel(struct net_if *iface);
#else
static inline int init_tunnel(void)
{
	return 0;
}

static inline bool is_tunnel(struct net_if *iface)
{
	ARG_UNUSED(iface);
	return false;
}
#endif /* CONFIG_NET_L2_IPIP */

#if defined(CONFIG_USB_DEVICE_STACK)
int init_usb(void);
#else
static inline int init_usb(void)
{
	return 0;
}
#endif /* CONFIG_USB_DEVICE_STACK */
