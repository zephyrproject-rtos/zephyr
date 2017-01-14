/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <net/dns_client.h>
#include <net/net_core.h>
#include <net/net_if.h>

#include <misc/printk.h>
#include <string.h>
#include <errno.h>

#include "config.h"

#define RC_STR(rc)	(rc == 0 ? "OK" : "ERROR")
#define MAX_ADDRESSES	4
#define STACK_SIZE	2048
uint8_t stack[STACK_SIZE];

static struct net_context *net_ctx;

#ifdef CONFIG_NET_IPV6
	struct in6_addr local_addr = LOCAL_ADDR;
	struct sockaddr_in6 local_sock = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(MY_PORT),
		.sin6_addr = LOCAL_ADDR };
	struct sockaddr_in6 remote_sock = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(PEER_PORT),
		.sin6_addr = REMOTE_ADDR };
	struct in6_addr addresses[MAX_ADDRESSES];
#else
	struct in_addr local_addr = LOCAL_ADDR;
	struct sockaddr_in local_sock = {
		.sin_family = AF_INET,
		.sin_port = htons(MY_PORT),
		.sin_addr = LOCAL_ADDR };
	struct sockaddr_in remote_sock = {
		.sin_family = AF_INET,
		.sin_port = htons(PEER_PORT),
		.sin_addr = REMOTE_ADDR };
	struct in_addr addresses[MAX_ADDRESSES];
#endif

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

/* from subsys/net/ip/utils.c */
char *net_sprint_ip_addr_buf(const uint8_t *ip, int ip_len,
			     char *buf, int buflen);

void run_dns(void)
{
	struct dns_context ctx;
	void *p;
	int rc;
	int d;
	int i;

	dns_init(&ctx);

#ifdef CONFIG_NET_IPV6
	p = net_if_ipv6_addr_add(net_if_get_default(), &local_addr,
				 NET_ADDR_MANUAL, 0);
#else
	p = net_if_ipv4_addr_add(net_if_get_default(), &local_addr,
				 NET_ADDR_MANUAL, 0);
#endif
	if (p == NULL) {
		printk("[%s:%d] Unable to add address to network interface\n",
		       __func__, __LINE__);
		return;
	}

#ifdef CONFIG_NET_IPV6
	rc = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &net_ctx);
#else
	rc = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &net_ctx);
#endif
	if (rc != 0) {
		printk("[%s:%d] Cannot get network context for IPv4 UDP: %d\n",
		       __func__, __LINE__, rc);
		return;
	}

#ifdef CONFIG_NET_IPV6
	rc = net_context_bind(net_ctx, (struct sockaddr *)&local_sock,
			      sizeof(struct sockaddr_in6));
#else
	rc = net_context_bind(net_ctx, (struct sockaddr *)&local_sock,
			      sizeof(struct sockaddr_in));
#endif
	if (rc != 0) {
		printk("[%s:%d] Cannot bind: %d\n",
		       __func__, __LINE__, rc);
		goto lb_exit;
	}

	ctx.net_ctx = net_ctx;
	ctx.timeout = APP_SLEEP_MSECS;
	ctx.dns_server = (struct sockaddr *)&remote_sock;
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
			net_sprint_ip_addr_buf(addresses[i].in6_u.u6_addr8,
					       16, str, sizeof(str));
#else
			net_sprint_ip_addr_buf(addresses[i].in4_u.u4_addr8,
					       4, str, sizeof(str));
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
