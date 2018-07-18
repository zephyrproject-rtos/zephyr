/*
 * Copyright (c) 2017 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define MY_PORT 4242
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#define STACK_SIZE 4096
#else
#define STACK_SIZE 1024
#endif
#define THREAD_PRIORITY K_PRIO_COOP(8)
#define RECV_BUFFER_SIZE 1280

struct data {
	const char *proto;

	struct {
		int sock;
		char recv_buffer[RECV_BUFFER_SIZE];
		u32_t counter;
	} udp;

	struct {
		int sock;
		char recv_buffer[RECV_BUFFER_SIZE];
		u32_t counter;
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
