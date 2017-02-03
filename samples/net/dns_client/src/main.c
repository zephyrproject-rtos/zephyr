/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <net/dns_client.h>
#include <net/net_core.h>
#include <net/net_if.h>

#include <misc/printk.h>
#include <string.h>
#include <errno.h>

#include "config.h"

#define STACK_SIZE	2048
uint8_t stack[STACK_SIZE];

#if CONFIG_NET_IPV6
	static struct in6_addr addresses[MAX_ADDRESSES];
#else
	static struct in_addr addresses[MAX_ADDRESSES];
#endif
static struct net_context *net_ctx;
static struct sockaddr remote_sock;
static char str[128];

static
char *domains[] = {"not_a_real_domain_name",
		   "zephyrproject.org",
		   "linux.org",
		   "www.zephyrproject.org",
		   "kernel.org",
		   "gerrit.zephyrproject.org",
		   "linuxfoundation.org",
		   "jira.zephyrproject.org",
		   "www.wikipedia.org",
		   "collabprojects.linuxfoundation.org",
		   "gcc.gnu.org",
		   "events.linuxfoundation.org",
		   "www.google.com",
		   NULL};

static
int set_addr(struct sockaddr *sock_addr, const char *addr, uint16_t port);

static
int set_local_addr(struct net_context **net_ctx, const char *local_addr);

void run_dns(void)
{
	struct dns_context ctx;
	int rc;
	int d;
	int i;

	rc = set_local_addr(&net_ctx, LOCAL_ADDR);
	if (rc) {
		return;
	}

	rc = set_addr(&remote_sock, REMOTE_ADDR, REMOTE_PORT);
	if (rc) {
		goto lb_exit;
	}

	dns_init(&ctx);

	ctx.net_ctx = net_ctx;
	ctx.timeout = APP_SLEEP_MSECS;
	ctx.dns_server = &remote_sock;
	ctx.elements = MAX_ADDRESSES;
#ifdef CONFIG_NET_IPV6
	ctx.query_type = DNS_QUERY_TYPE_AAAA;
	ctx.address.ipv6 = addresses;
#else
	ctx.query_type = DNS_QUERY_TYPE_A;
	ctx.address.ipv4 = addresses;
#endif

	for (d = 0; domains[d] != NULL; d++) {

		printk("\n-------------------------------------------\n"
		       "[%s:%d] name: %s\n", __func__, __LINE__, domains[d]);

		ctx.name = domains[d];
		rc = dns_resolve(&ctx);
		if (rc != 0) {
			printk("rc: %d\n", rc);
			continue;
		}

		for (i = 0; i < ctx.items; i++) {
#ifdef CONFIG_NET_IPV6
			net_addr_ntop(AF_INET6, &addresses[i],
				      str, sizeof(str));
#else
			net_addr_ntop(AF_INET, &addresses[i],
				      str, sizeof(str));
#endif
			printk("[%s:%d] %s\n", __func__, __LINE__, str);
		}

		k_sleep(APP_SLEEP_MSECS);
	}

lb_exit:
	net_context_put(net_ctx);
	printk("\nBye!\n");
}

void main(void)
{
	k_thread_spawn(stack, STACK_SIZE, (k_thread_entry_t)run_dns,
		       NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}

static
int set_addr(struct sockaddr *sock_addr, const char *addr, uint16_t port)
{
	void *ptr;
	int rc;

#ifdef CONFIG_NET_IPV6
	net_sin6(sock_addr)->sin6_port = htons(port);
	sock_addr->family = AF_INET6;
	ptr = &(net_sin6(sock_addr)->sin6_addr);
	rc = net_addr_pton(AF_INET6, addr, ptr);
#else
	net_sin(sock_addr)->sin_port = htons(port);
	sock_addr->family = AF_INET;
	ptr = &(net_sin(sock_addr)->sin_addr);
	rc = net_addr_pton(AF_INET, addr, ptr);
#endif

	if (rc) {
		printk("Invalid IP address: %s\n", addr);
	}

	return rc;
}

static
int set_local_addr(struct net_context **net_ctx, const char *local_addr)
{
#ifdef CONFIG_NET_IPV6
	socklen_t addr_len = sizeof(struct sockaddr_in6);
	sa_family_t family = AF_INET6;

#else
	socklen_t addr_len = sizeof(struct sockaddr_in);
	sa_family_t family = AF_INET;
#endif
	struct sockaddr local_sock;
	void *p;
	int rc;

	rc = set_addr(&local_sock, local_addr, 0);
	if (rc) {
		printk("set_addr (local) error\n");
		return rc;
	}

#ifdef CONFIG_NET_IPV6
	p = net_if_ipv6_addr_add(net_if_get_default(),
				 &net_sin6(&local_sock)->sin6_addr,
				 NET_ADDR_MANUAL, 0);
#else
	p = net_if_ipv4_addr_add(net_if_get_default(),
				 &net_sin(&local_sock)->sin_addr,
				 NET_ADDR_MANUAL, 0);
#endif

	if (!p) {
		return -EINVAL;
	}

	rc = net_context_get(family, SOCK_DGRAM, IPPROTO_UDP, net_ctx);
	if (rc) {
		printk("net_context_get error\n");
		return rc;
	}

	rc = net_context_bind(*net_ctx, &local_sock, addr_len);
	if (rc) {
		printk("net_context_bind error\n");
		goto lb_exit;
	}

	return 0;

lb_exit:
	net_context_put(*net_ctx);

	return rc;
}
