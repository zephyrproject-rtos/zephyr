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

#include <net/ip_buf.h>
#include <net/net_core.h>
#include <net/net_socket.h>

#if defined(CONFIG_NETWORKING_WITH_BT)
#include <bluetooth/bluetooth.h>
#include <gatt/ipss.h>
#endif

#if defined(CONFIG_NET_TESTING)
#include <net_testing.h>
#else
#if defined(CONFIG_NETWORKING_WITH_IPV6)
#include <contiki/ipv6/uip-ds6.h>
#else
#include <contiki/ip/uipaddr.h>
#endif /* IPv6 */
#endif /* CONFIG_NET_TESTING */

#if defined(CONFIG_NETWORKING_WITH_IPV6)
/* admin-local, dynamically allocated multicast address */
#define MCAST_IPADDR { { { 0xff, 0x84, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x2 } } }

/* Define my IP address where to expect messages */
#if !defined(CONFIG_NET_TESTING)
#define MY_IPADDR { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x2 } } }
#define MY_PREFIX_LEN 64
#endif

#if defined(CONFIG_NET_TESTING)
static const struct in6_addr in6addr_my = IN6ADDR_ANY_INIT;
#else
static const struct in6_addr in6addr_my = MY_IPADDR;
#endif

#else /* IPv6 */

/* Organization-local 239.192.0.0/14 */
#define MCAST_IPADDR { { { 239, 192, 0, 2 } } }

#if !defined(CONFIG_NET_TESTING)
/* The 192.0.2.0/24 is the private address space for documentation RFC 5737 */
#define UIP_IPADDR0 192
#define UIP_IPADDR1 0
#define UIP_IPADDR2 2
#define UIP_IPADDR3 2

#define UIP_DRIPADDR0 UIP_IPADDR0
#define UIP_DRIPADDR1 UIP_IPADDR1
#define UIP_DRIPADDR2 UIP_IPADDR2
#define UIP_DRIPADDR3 42

#define MY_IPADDR { { { UIP_IPADDR0, UIP_IPADDR1, UIP_IPADDR2, UIP_IPADDR3 } } }

uip_ipaddr_t uip_hostaddr = { { UIP_IPADDR0, UIP_IPADDR1,
				UIP_IPADDR2, UIP_IPADDR3 } };
uip_ipaddr_t uip_draddr = { { UIP_DRIPADDR0, UIP_DRIPADDR1,
			      UIP_DRIPADDR2, UIP_DRIPADDR3 } };
uip_ipaddr_t uip_netmask = { { 255, 255, 255, 0 } };
#endif /* CONFIG_NET_TESTING */

#endif /* IPv6 */

#define MY_PORT 4242

static inline void init_app(void)
{
	PRINT("%s: run echo server\n", __func__);

#if defined(CONFIG_NET_TESTING)
	net_testing_setup();
#else
#if defined(CONFIG_NETWORKING_WITH_IPV6)
	uip_ds6_prefix_add((uip_ipaddr_t *)&in6addr_my, MY_PREFIX_LEN, 0);
#endif
#endif
}

static inline void reverse(unsigned char *buf, int len)
{
	int i, last = len - 1;

	for (i = 0; i < len / 2; i++) {
		unsigned char tmp = buf[i];
		buf[i] = buf[last - i];
		buf[last - i] = tmp;
	}
}

static inline struct net_buf *prepare_reply(const char *name,
					    const char *type,
					    struct net_buf *buf,
					    int proto)
{
	PRINT("%s: %sreceived %d bytes data\n", name, type,
	      ip_buf_appdatalen(buf));

	if (proto != IPPROTO_TCP) {
		/* In this test we reverse the received bytes.
		 * We could just pass the data back as is but
		 * this way it is possible to see how the app
		 * can manipulate the received data.
		 */
		reverse(ip_buf_appdata(buf), ip_buf_appdatalen(buf));
	}

#if defined(CONFIG_NET_TESTING)
	net_testing_set_reply_address(buf);
#endif

	return buf;
}

/* How many tics to wait for a network packet */
#if 1
#define WAIT_TIME 0
#define WAIT_TICKS (WAIT_TIME * sys_clock_ticks_per_sec / 10)
#else
#define WAIT_TICKS TICKS_UNLIMITED
#endif

struct nano_fifo *net_context_get_queue(struct net_context *context);
static inline void receive_and_reply(const char *name,
				     struct net_context *udp_recv,
				     struct net_context *tcp_recv,
				     struct net_context *mcast_recv)
{
	struct net_buf *buf;

	buf = net_receive(udp_recv, WAIT_TICKS);
	if (buf) {
		prepare_reply(name, "unicast ", buf, IPPROTO_UDP);

		if (net_reply(udp_recv, buf)) {
			ip_buf_unref(buf);
		}
		return;
	}

	buf = net_receive(mcast_recv, WAIT_TICKS);
	if (buf) {
		prepare_reply(name, "multicast ", buf, IPPROTO_UDP);

		if (net_reply(mcast_recv, buf)) {
			ip_buf_unref(buf);
		}
		return;
	}

#if defined(CONFIG_NETWORKING_WITH_TCP)
	if (tcp_recv) {
		static struct net_buf *tcpbuf;

		if (tcpbuf) {
			int ret;
		reply:
			ret = net_reply(tcp_recv, tcpbuf);
			if (ret && ret != -EAGAIN) {
				ip_buf_unref(tcpbuf);
				tcpbuf = NULL;
			} else if (!ret) {
				tcpbuf = NULL;
			} else {
				PRINT("Retrying to send packet %p\n", tcpbuf);
			}
		} else {
			tcpbuf = net_receive(tcp_recv, WAIT_TICKS);
			if (tcpbuf) {
				PRINT("Received packet %p len %d\n",
				      tcpbuf, ip_buf_appdatalen(tcpbuf));
				prepare_reply(name, "tcp ", tcpbuf, IPPROTO_TCP);
				goto reply;
			}
		}
	}
#endif

	fiber_sleep(50);
}

static inline bool get_context(struct net_context **udp_recv,
			       struct net_context **tcp_recv,
			       struct net_context **mcast_recv)
{
	static struct net_addr mcast_addr;
	static struct net_addr any_addr;
	static struct net_addr my_addr;

#if defined(CONFIG_NETWORKING_WITH_IPV6)
	static const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
	static const struct in6_addr in6addr_mcast = MCAST_IPADDR;

	mcast_addr.in6_addr = in6addr_mcast;
	mcast_addr.family = AF_INET6;

	any_addr.in6_addr = in6addr_any;
	any_addr.family = AF_INET6;

	my_addr.in6_addr = in6addr_my;
	my_addr.family = AF_INET6;
#else
	static const struct in_addr in4addr_any = { { { 0 } } };
	static struct in_addr in4addr_my = MY_IPADDR;
	static struct in_addr in4addr_mcast = MCAST_IPADDR;

	mcast_addr.in_addr = in4addr_mcast;
	mcast_addr.family = AF_INET;

	any_addr.in_addr = in4addr_any;
	any_addr.family = AF_INET;

	my_addr.in_addr = in4addr_my;
	my_addr.family = AF_INET;
#endif

	*udp_recv = net_context_get(IPPROTO_UDP,
				    &any_addr, 0,
				    &my_addr, MY_PORT);
	if (!*udp_recv) {
		PRINT("%s: Cannot get network context\n", __func__);
		return NULL;
	}

#if defined(CONFIG_NETWORKING_WITH_TCP)
	if (tcp_recv) {
		*tcp_recv = net_context_get(IPPROTO_TCP,
					    &any_addr, 0,
					    &my_addr, MY_PORT);
		if (!*tcp_recv) {
			PRINT("%s: Cannot get network context\n", __func__);
			return NULL;
		}
	}
#endif

	*mcast_recv = net_context_get(IPPROTO_UDP,
				      &any_addr, 0,
				      &mcast_addr, MY_PORT);
	if (!*mcast_recv) {
		PRINT("%s: Cannot get receiving mcast network context\n",
		      __func__);
	}

	return true;
}

#if defined(CONFIG_NANOKERNEL)
#define STACKSIZE 2000
char __noinit __stack fiberStack[STACKSIZE];
#endif

void receive(void)
{
	static struct net_context *udp_recv, *tcp_recv;
	static struct net_context *mcast_recv;

	if (!get_context(&udp_recv, &tcp_recv, &mcast_recv)) {
		PRINT("%s: Cannot get network contexts\n", __func__);
		return;
	}

	while (1) {
		receive_and_reply(__func__, udp_recv, tcp_recv, mcast_recv);
	}
}

void main(void)
{
	net_init();

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
	task_fiber_start (&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t)receive, 0, 0, 7, 0);
#endif
}
