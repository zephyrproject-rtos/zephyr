/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_utils.h"
#include <stdio.h>

#include "config.h"

#define IP6_SIZE	16
#define IP4_SIZE	4

#define STR_IP6_ADDR	64
#define STR_IP4_ADDR	16

static
void print_ipaddr(const struct sockaddr *ip_addr)
{
#ifdef CONFIG_NET_IPV6
	struct sockaddr_in6 *addr = (struct sockaddr_in6 *)ip_addr;
	void *raw = (void *)&addr->sin6_addr;
	uint16_t port = addr->sin6_port;
	sa_family_t family = AF_INET6;
	char str[STR_IP6_ADDR];
#else
	struct sockaddr_in *addr = (struct sockaddr_in *)ip_addr;
	void *raw = (void *)&addr->sin_addr;
	uint16_t port = addr->sin_port;
	sa_family_t family = AF_INET;
	char str[STR_IP4_ADDR];
#endif

	net_addr_ntop(family, raw, str, sizeof(str));
	printf("Address: %s, port: %d\n", str, ntohs(port));
}

void print_client_banner(const struct sockaddr *addr)
{
	printf("\n----------------------------------------------------\n");
	printf("[%s:%d] Connection accepted\n", __func__, __LINE__);
	print_ipaddr(addr);
}


void print_server_banner(const struct sockaddr *addr)
{
	printf("Zephyr HTTP Server\n");
	print_ipaddr(addr);
}
