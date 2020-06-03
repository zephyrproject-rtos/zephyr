/*
 * Copyright (c) 2017-2019 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define MY_PORT 4242
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) || defined(CONFIG_NET_TCP2) || \
	defined(CONFIG_COVERAGE)
#define STACK_SIZE 4096
#else
#define STACK_SIZE 1024
#endif
#define THREAD_PRIORITY K_PRIO_COOP(8)
#define RECV_BUFFER_SIZE 1280

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
		char recv_buffer[RECV_BUFFER_SIZE];
		uint32_t counter;
	} udp;

	struct {
		int sock;

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
