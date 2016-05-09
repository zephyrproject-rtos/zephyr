/** @file
 * @brief Network initialization
 *
 * Initialize the network IP stack. Create two fibers, one for reading data
 * from applications (Tx fiber) and one for reading data from IP stack
 * and passing that data to applications (Rx fiber).
 */

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

#if defined(CONFIG_NETWORK_IP_STACK_DEBUG_CORE)
#define SYS_LOG_DOMAIN "net/core"
#define NET_DEBUG 1
#endif

#include <init.h>
#include <nanokernel.h>
#include <toolchain.h>
#include <sections.h>
#include <string.h>
#include <errno.h>

#include <net/net_if.h>
#include <net/nbuf.h>
#include <net/net_core.h>

#include "net_private.h"

/* Stack for the rx fiber.
 */
#if !defined(CONFIG_NET_RX_STACK_SIZE)
#define CONFIG_NET_RX_STACK_SIZE 1024
#endif
static char __noinit __stack rx_fiber_stack[CONFIG_NET_RX_STACK_SIZE];
static struct nano_fifo rx_queue;

static void net_rx_fiber(int unused1, int unused2)
{
	while (0) {
		/* FIXME - implementation missing */
	}
}

static void init_rx_queue(void)
{
	nano_fifo_init(&rx_queue);

	fiber_start(rx_fiber_stack, sizeof(rx_fiber_stack),
		    net_rx_fiber, 0, 0, 7, 0);
}

/* Called by driver when an IP packet has been received */
int net_recv(struct net_if *iface, struct net_buf *buf)
{
	if (!buf->frags) {
		return -ENODATA;
	}

	nano_fifo_put(&rx_queue, buf);

	return 0;
}

static int network_initialization(void)
{

	return 0;
}

static int net_init(struct device *unused)
{
	static bool is_initialized;

	if (is_initialized)
		return -EALREADY;

	is_initialized = true;

	NET_DBG("Priority %d", CONFIG_NET_INIT_PRIO);

	net_nbuf_init();

	init_rx_queue();

	net_if_init();

	return network_initialization();
}

SYS_INIT(net_init, NANOKERNEL, CONFIG_NET_INIT_PRIO);
