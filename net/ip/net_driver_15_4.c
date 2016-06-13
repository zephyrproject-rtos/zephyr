/* net_driver_15_4.c - IP 15.4 driver */

/*
 * Copyright (c) 2015 Intel Corporation
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

#include <nanokernel.h>
#include <toolchain.h>
#include <sections.h>
#include <string.h>
#include <errno.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#include "contiki/ip/uip-debug.h"

#include <net/net_core.h>
#include <net/l2_buf.h>
#include <net/net_ip.h>
#include <net/net_socket.h>
#include "contiki/netstack.h"
#include <net_driver_15_4.h>

#if !defined(CONFIG_NETWORK_IP_STACK_DEBUG_15_4_NET_DRIVER)
#undef NET_DBG
#define NET_DBG(...)
#endif

/* Stacks for the tx & rx fibers.
 * FIXME: stack size needs fine-tuning
 */
#define STACKSIZE_UNIT 1024

#ifndef CONFIG_15_4_RX_STACK_SIZE
#define CONFIG_15_4_RX_STACK_SIZE (STACKSIZE_UNIT * 1)
#endif
static char __noinit __stack rx_fiber_stack[CONFIG_15_4_RX_STACK_SIZE];

/* Queue for incoming packets from hw driver */
static struct nano_fifo rx_queue;

static int net_driver_15_4_open(void)
{
	return 0;
}

static int net_driver_15_4_send(struct net_buf *buf)
{
#if defined(CONFIG_NETWORK_IP_STACK_DEBUG_15_4_NET_DRIVER)
	int orig_len = ip_buf_len(buf);
#endif

	if (!NETSTACK_COMPRESS.compress(buf)) {
		NET_DBG("compression failed\n");
		ip_buf_unref(buf);
		return -EINVAL;
	}

	NET_DBG("sending %d bytes (original len %d)\n", ip_buf_len(buf),
		orig_len);

	if (uip_len(buf) == 0) {
		/* It is possible that uIP stack overwrote the len.
		 * We need to fix this here.
		 */
		uip_len(buf) = ip_buf_len(buf);
	}

	NET_DBG("Sending (%u bytes) to 15.4 stack\n", ip_buf_len(buf));

	if (!NETSTACK_FRAGMENT.fragment(buf, NULL)) {
		/* Release buffer on error */
		ip_buf_unref(buf);
	}

	return 1;
}

static void net_rx_15_4_fiber(void)
{
	struct net_buf *buf;
#if NET_MAC_CONF_STATS
	int byte_count;
#endif

	NET_DBG("Starting 15.4 RX fiber (stack %d bytes)\n",
		sizeof(rx_fiber_stack));

	while (1) {
		/* Wait next packet from 15.4 stack */
		buf = net_buf_get_timeout(&rx_queue, 0, TICKS_UNLIMITED);

#if NET_MAC_CONF_STATS
		byte_count = uip_pkt_buflen(buf);
#endif
		if (!NETSTACK_RDC.input(buf)) {
			NET_DBG("802.15.4 RDC input failed, "
				"buf %p discarded\n", buf);
			l2_buf_unref(buf);
		} else {
#if NET_MAC_CONF_STATS
			net_mac_stats.bytes_received += byte_count;
#endif
		}

		net_analyze_stack("802.15.4 RX", rx_fiber_stack,
				  sizeof(rx_fiber_stack));
	}
}

static void init_rx_queue(void)
{
        nano_fifo_init(&rx_queue);

	fiber_start(rx_fiber_stack, sizeof(rx_fiber_stack),
		    (nano_fiber_entry_t) net_rx_15_4_fiber, 0, 0, 7, 0);
}

static struct net_driver net_driver_15_4 = {
	.head_reserve = 0,
	.open = net_driver_15_4_open,
	.send = net_driver_15_4_send,
};

int net_driver_15_4_init(void)
{
	init_rx_queue();

	NETSTACK_RADIO.init();
	NETSTACK_RDC.init();
	NETSTACK_MAC.init();
	NETSTACK_COMPRESS.init();

	net_register_driver(&net_driver_15_4);

	return 0;
}

int net_driver_15_4_recv(struct net_buf *buf)
{
	if (!uip_uncompressed(buf)) {
		if (!NETSTACK_COMPRESS.uncompress(buf)) {
			return -EINVAL;
		}
	}

	if (net_recv(buf) < 0) {
		NET_DBG("input to IP stack failed\n");
		return -EINVAL;
	}

	return 0;
}

int net_driver_15_4_recv_from_hw(struct net_buf *buf)
{
	nano_fifo_put(&rx_queue, buf);
	return 0;
}
