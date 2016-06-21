/* echo.c - Networking echo server */

/*
 * Copyright (c) 2015 Intel Corporation.
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

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#include <zephyr.h>
#include <sections.h>
#include <errno.h>

#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>

#if defined(CONFIG_NETWORKING_WITH_BT)
#include <bluetooth/bluetooth.h>
#include <gatt/ipss.h>
#endif

#if defined(CONFIG_NET_TESTING)
#include <net_testing.h>
#else
#endif /* CONFIG_NET_TESTING */

#if defined(CONFIG_NET_IPV6)
/* admin-local, dynamically allocated multicast address */
#define MCAST_IP6ADDR { { { 0xff, 0x84, 0, 0, 0, 0, 0, 0, \
			    0, 0, 0, 0, 0, 0, 0, 0x2 } } }

/* Define my IP address where to expect messages */
#if !defined(CONFIG_NET_TESTING)
#define MY_IP6ADDR { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, \
			 0, 0, 0, 0, 0, 0, 0, 0x1 } } }
#define MY_PREFIX_LEN 64
#endif

#if defined(CONFIG_NET_TESTING)
static const struct in6_addr in6addr_my = IN6ADDR_ANY_INIT;
#else
static const struct in6_addr in6addr_my = MY_IP6ADDR;
#endif

#endif /* IPv6 */

#if defined(CONFIG_NET_IPV4)
/* Organization-local 239.192.0.0/14 */
#define MCAST_IP4ADDR { { { 239, 192, 0, 2 } } }

/* The 192.0.2.0/24 is the private address space for documentation RFC 5737 */
#define MY_IP4ADDR { { { 192, 0, 2, 1 } } }
#endif /* IPv4 */

#define MY_PORT 4242

struct in6_addr addr6 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
			      0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct in_addr in4addr_my = MY_IP4ADDR;

static inline void init_app(void)
{
	PRINT("%s: run echo server\n", __func__);

#if defined(CONFIG_NET_TESTING)
	net_testing_setup();
#endif

#if defined(CONFIG_NET_IPV6)
	net_if_ipv6_addr_add(net_if_get_default(), &addr6, NET_ADDR_MANUAL, 0);
#endif

#if defined(CONFIG_NET_IPV4)
	net_if_ipv4_addr_add(net_if_get_default(), &in4addr_my, NET_ADDR_MANUAL, 0);
#endif
}


/* How many tics to wait for a network packet */
#if 1
#define WAIT_TIME 0
#define WAIT_TICKS (WAIT_TIME * sys_clock_ticks_per_sec / 10)
#else
#define WAIT_TICKS TICKS_UNLIMITED
#endif

static inline bool get_context(struct net_context **udp_recv4,
			       struct net_context **udp_recv6,
			       struct net_context **tcp_recv4,
			       struct net_context **tcp_recv6,
			       struct net_context **mcast_recv4,
			       struct net_context **mcast_recv6)
{
#if defined(CONFIG_NET_IPV6)
	static struct net_addr mcast_addr6;
	static struct net_addr any_addr6;
	static struct net_addr my_addr6;
	static const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
	static const struct in6_addr in6addr_mcast = MCAST_IP6ADDR;
#endif

#if defined(CONFIG_NET_IPV4)
	static struct net_addr mcast_addr4;
	static struct net_addr any_addr4;
	static struct net_addr my_addr4;
	static const struct in_addr in4addr_any = { { { 0 } } };
	static struct in_addr in4addr_mcast = MCAST_IP4ADDR;
#endif

#if defined(CONFIG_NET_IPV6)
	mcast_addr6.in6_addr = in6addr_mcast;
	mcast_addr6.family = AF_INET6;

	any_addr6.in6_addr = in6addr_any;
	any_addr6.family = AF_INET6;

	my_addr6.in6_addr = in6addr_my;
	my_addr6.family = AF_INET6;
#endif

#if defined(CONFIG_NET_IPV4)
	mcast_addr4.in_addr = in4addr_mcast;
	mcast_addr4.family = AF_INET;

	any_addr4.in_addr = in4addr_any;
	any_addr4.family = AF_INET;

	my_addr4.in_addr = in4addr_my;
	my_addr4.family = AF_INET;
#endif

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_UDP)
	*udp_recv6 = net_context_get(IPPROTO_UDP,
				     &any_addr6, 0,
				     &my_addr6, MY_PORT);
	if (!*udp_recv6) {
		PRINT("%s: Cannot get network context\n", __func__);
		return NULL;
	}

	*mcast_recv6 = net_context_get(IPPROTO_UDP,
				       &any_addr6, 0,
				       &mcast_addr6, MY_PORT);
	if (!*mcast_recv6) {
		PRINT("%s: Cannot get receiving mcast network context\n",
		      __func__);
		return false;
	}

#endif

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_UDP)
	*udp_recv4 = net_context_get(IPPROTO_UDP,
				    &any_addr4, 0,
				    &my_addr4, MY_PORT);
	if (!*udp_recv4) {
		PRINT("%s: Cannot get network context\n", __func__);
		return NULL;
	}

	*mcast_recv4 = net_context_get(IPPROTO_UDP,
				       &any_addr4, 0,
				       &mcast_addr4, MY_PORT);
	if (!*mcast_recv4) {
		PRINT("%s: Cannot get receiving mcast network context\n",
		      __func__);
		return false;
	}
#endif

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_TCP)
	if (tcp_recv6) {
		*tcp_recv6 = net_context_get(IPPROTO_TCP,
					     &any_addr6, 0,
					     &my_addr6, MY_PORT);
		if (!*tcp_recv6) {
			PRINT("%s: Cannot get network context\n", __func__);
			return NULL;
		}
	}
#endif

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_TCP)
	if (tcp_recv4) {
		*tcp_recv4 = net_context_get(IPPROTO_TCP,
					     &any_addr4, 0,
					     &my_addr4, MY_PORT);
		if (!*tcp_recv4) {
			PRINT("%s: Cannot get network context\n", __func__);
			return NULL;
		}
	}
#endif

	return true;
}

#if defined(CONFIG_NANOKERNEL)
#define STACKSIZE 2000
char __noinit __stack fiberStack[STACKSIZE];
#endif

void receive(void)
{
	static struct net_context *udp_recv4, *udp_recv6,
		*tcp_recv4, *tcp_recv6;
	static struct net_context *mcast_recv4, *mcast_recv6;

	if (!get_context(&udp_recv4, &udp_recv6,
			 &tcp_recv4, &tcp_recv6,
			 &mcast_recv4, &mcast_recv6)) {
		PRINT("%s: Cannot get network contexts\n", __func__);
		return;
	}

	while (1) {
		/* FIXME - Implementation is missing */
		fiber_sleep(100);
	}
}

void main(void)
{
	init_app();

#if defined(CONFIG_NETWORKING_WITH_BT)
	if (bt_enable(NULL)) {
		PRINT("Bluetooth init failed\n");
		return;
	}
	ipss_init();
	ipss_advertise();
#endif

#if defined(CONFIG_MICROKERNEL)
	receive();
#else
	task_fiber_start(&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t)receive, 0, 0, 7, 0);
#endif
}
