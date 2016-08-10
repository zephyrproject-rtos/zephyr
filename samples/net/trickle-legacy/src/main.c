/* main.c - Trickle sample app for legacy IP stack */

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
#include <misc/nano_work.h>

#include <net/ip_buf.h>
#include <net/net_core.h>
#include <net/net_socket.h>

#include <bluetooth/bluetooth.h>
#include <gatt/ipss.h>

#include <contiki/ipv6/uip-ds6.h>
#include <contiki/trickle/trickle-timer.h>

#define MY_IPADDR { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0x1 } } }
#define MY_PREFIX_LEN 64

static const struct in6_addr in6addr_my = MY_IPADDR;
#define MY_PORT 30001

#if defined(CONFIG_NANOKERNEL)
#define STACKSIZE 2000
char __noinit __stack fiberStack[STACKSIZE];
#endif

static struct nano_delayed_work token_lifetime;
static uint32_t timeout = SECONDS(5);

static struct trickle_timer tt;
static uint8_t token;

#define IMIN             16   /* ticks */
#define IMAX             10   /* doublings */
#define REDUNDANCY_CONST 2

static void trickle_tx(void *user_data, uint8_t suppress)
{
	struct trickle_timer *t = (struct trickle_timer *)user_data;

	if (suppress == TRICKLE_TIMER_TX_SUPPRESS) {
		return;
	}

	PRINT("At %lu (I=%lu, c=%u): token 0x%02x\n",
	      sys_tick_get_32(), (unsigned long)t->i_cur,
	      t->c, token);

	/* FIXME: Send token .... */
}

static inline void init_app(void)
{
	PRINT("%s: run trickle demo\n", __func__);

	token = 0;

	trickle_timer_config(&tt, IMIN, IMAX, REDUNDANCY_CONST);
	trickle_timer_set(&tt, trickle_tx, &tt);

	uip_ds6_prefix_add((uip_ipaddr_t *)&in6addr_my, MY_PREFIX_LEN, 0);
}

/* How many tics to wait for a network packet */
#define WAIT_TIME 1
#define WAIT_TICKS (WAIT_TIME * sys_clock_ticks_per_sec)

struct nano_fifo *net_context_get_queue(struct net_context *context);
static inline void receive_and_reply(const char *name,
				     struct net_context *udp_recv)
{
	struct net_buf *buf;

	buf = net_receive(udp_recv, WAIT_TICKS);
	if (buf) {
	}

	fiber_sleep(50);
}

static inline bool get_context(struct net_context **udp_recv)
{
	static struct net_addr any_addr;
	static struct net_addr my_addr;
	static const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;

	any_addr.in6_addr = in6addr_any;
	any_addr.family = AF_INET6;

	my_addr.in6_addr = in6addr_my;
	my_addr.family = AF_INET6;

	*udp_recv = net_context_get(IPPROTO_UDP,
				    &any_addr, 0,
				    &my_addr, MY_PORT);
	if (!*udp_recv) {
		PRINT("%s: Cannot get network context\n", __func__);
		return false;
	}

	return true;
}

static inline void token_expired(struct nano_work *work)
{
	PRINT("Generate a new token value %d\n", token);

	/* Periodically (and randomly) generate a new token.
	 * This will trigger a trickle inconsistency
	 */
	if (!(sys_rand32_get() % 2)) {
		token++;

		PRINT("Generating a new token 0x%02x\n", token);

		trickle_timer_reset_event(&tt);
	}

	nano_delayed_work_submit(&token_lifetime, timeout);
}

void receive(void)
{
	static struct net_context *udp_recv;

	if (!get_context(&udp_recv)) {
		PRINT("%s: Cannot get network contexts\n", __func__);
		return;
	}

	/*
	 * Trickle is now started and is running the first interval.
	 * All nodes agree that token is 0 initially. This will then
	 * change when one node randomly decides to generate a new one token.
	 */
	nano_delayed_work_init(&token_lifetime, token_expired);
	nano_delayed_work_submit(&token_lifetime, timeout);

	while (1) {
		receive_and_reply(__func__, udp_recv);
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
