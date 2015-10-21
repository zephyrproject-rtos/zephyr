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

/*
 * Note that both the nano and microkernel images in this example
 * have a dummy fiber/task that does nothing. This is just here to
 * simulate a multi application scenario.
 */

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#include <zephyr.h>

#include <net/net_core.h>
#include <net/net_socket.h>

/* The peer is the client in our case. Just invent a mac
 * address for it because lower parts of the stack cannot set it
 * in this test as we do not have any radios.
 */
static uint8_t peer_mac[] = { 0x15, 0x0a, 0xbe, 0xef, 0xf0, 0x0d };

/* This is my mac address
 */
static uint8_t my_mac[] = { 0x0a, 0xbe, 0xef, 0x15, 0xf0, 0x0d };

#ifdef CONFIG_NETWORKING_WITH_IPV6
/* The 2001:db8::/32 is the private address space for documentation RFC 3849 */
#define MY_IPADDR { { { 0x20,0x01,0x0d,0xb8,0,0,0,0,0,0,0,0,0,0,0,0x2 } } }

/* admin-local, dynamically allocated multicast address */
#define MCAST_IPADDR { { { 0xff,0x84,0,0,0,0,0,0,0,0,0,0,0,0,0,0x2 } } }
#else
/* The 192.0.2.0/24 is the private address space for documentation RFC 5737 */
#define MY_IPADDR { { { 192,0,2,2 } } }

/* Organization-local 239.192.0.0/14 */
#define MCAST_IPADDR { { { 239,192,0,2 } } }
#endif
#define MY_PORT 4242

static inline void init_server()
{
	PRINT("%s: run echo server\n", __func__);

	net_set_mac(my_mac, sizeof(my_mac));

#ifdef CONFIG_NETWORKING_WITH_IPV4
	{
		uip_ipaddr_t addr;
		uip_ipaddr(&addr, 192,0,2,2);
		uip_sethostaddr(&addr);
	}
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
					    struct net_buf *buf)
{
	PRINT("%s: %sreceived %d bytes\n", name, type, net_buf_datalen(buf));

	/* In this test we reverse the received bytes.
	 * We could just pass the data back as is but
	 * this way it is possible to see how the app
	 * can manipulate the received data.
	 */
	reverse(net_buf_data(buf), net_buf_datalen(buf));

	/* Set the mac address of the peer in net_buf because
	 * there is no radio layer involved in this test app.
	 * Normally there is no need to do this.
	 */
	memcpy(&buf->src, &peer_mac, sizeof(buf->src));

	return buf;
}

/* How many tics to wait for a network packet */
#if 0
#define WAIT_TIME 1
#define WAIT_TICKS (WAIT_TIME * sys_clock_ticks_per_sec)
#else
#define WAIT_TICKS TICKS_UNLIMITED
#endif

static inline void receive_and_reply(const char *name, struct net_context *recv,
				     struct net_context *mcast_recv)
{
	struct net_buf *buf;

	buf = net_receive(recv, WAIT_TICKS);
	if (buf) {
		prepare_reply(name, "unicast ", buf);

		if (net_reply(recv, buf)) {
			net_buf_put(buf);
		}
		return;
	}

	buf = net_receive(mcast_recv, WAIT_TICKS);
	if (buf) {
		prepare_reply(name, "multicast ", buf);

		if (net_reply(mcast_recv, buf)) {
			net_buf_put(buf);
		}
		return;
	}
}

static inline bool get_context(struct net_context **recv,
			       struct net_context **mcast_recv)
{
	static struct net_addr mcast_addr;
	static struct net_addr any_addr;
	static struct net_addr my_addr;

#ifdef CONFIG_NETWORKING_WITH_IPV6
	static const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
	static const struct in6_addr in6addr_mcast = MCAST_IPADDR;
	static struct in6_addr in6addr_my = MY_IPADDR;

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

	*recv = net_context_get(IPPROTO_UDP,
				&any_addr, 0,
				&my_addr, MY_PORT);
	if (!*recv) {
		PRINT("%s: Cannot get network context\n", __func__);
		return NULL;
	}

	*mcast_recv = net_context_get(IPPROTO_UDP,
				      &any_addr, 0,
				      &mcast_addr, MY_PORT);
	if (!*mcast_recv) {
		PRINT("%s: Cannot get receiving mcast network context\n",
		      __func__);
		return false;
	}

	return true;
}

#ifdef CONFIG_MICROKERNEL

void task_receive(void)
{
	static struct net_context *recv;
	static struct net_context *mcast_recv;

	net_init();

	init_server();

	if (!get_context(&recv, &mcast_recv)) {
		PRINT("%s: Cannot get network contexts\n", __func__);
		return;
	}

	while (1) {
		receive_and_reply(__func__, recv, mcast_recv);
	}
}

#else /*  CONFIG_NANOKERNEL */

#define STACKSIZE 2000

char fiberStack[STACKSIZE];

void fiber_receive(void)
{
	static struct net_context *recv;
	static struct net_context *mcast_recv;

	if (!get_context(&recv, &mcast_recv)) {
		PRINT("%s: Cannot get network contexts\n", __func__);
		return;
	}

	while (1) {
		receive_and_reply(__func__, recv, mcast_recv);
	}
}

void main(void)
{
	net_init();

	init_server();

	task_fiber_start (&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t)fiber_receive, 0, 0, 7, 0);
}

#endif /* CONFIG_MICROKERNEL ||  CONFIG_NANOKERNEL */
