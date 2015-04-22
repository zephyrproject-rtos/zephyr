/*! @file
 @brief Network initialization

 Initialize the network IP stack. Create two fibers, one for reading data
 from applications (Tx fiber) and one for reading data from IP stack
 and passing that data to applications (Rx fiber).
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define DEBUG DEBUG_PRINT
#include "contiki/ip/uip-debug.h"

#include <nanokernel.h>
#include <toolchain.h>
#include <string.h>
#include <errno.h>

#include <net/net_core.h>
#include <net/net_buf.h>

#include "net/core/netstack.h"
#include "net/core/uip-driver.h"

/* Stacks for the tx & rx fibers.
 * FIXME: stack size needs fine-tuning
 */
#define STACKSIZE 2048
static char rx_fiber_stack[STACKSIZE];
static char tx_fiber_stack[STACKSIZE];

struct net_context {
	/* Application receives data via this fifo */
	struct nano_fifo rx_queue;
};

static struct net_dev {
	/* Queue for incoming packets from driver */
	struct nano_fifo rx_queue;

	/* Queue for outgoing packets from apps */
	struct nano_fifo tx_queue;

	/* Registered network driver, FIXME: how this is set? */
	struct net_driver *drv;
} netdev;

static void net_tx_fiber(void)
{
	struct net_driver *drv = netdev.drv;

	NET_DBG("\n");

	while (1) {
		struct net_buf *buf;

		/* Get next packet from application - wait if necessary */
		buf = nano_fifo_get_wait(&netdev.tx_queue);

		NET_DBG("Sending (buf %p, len %u) to stack\n", buf, buf->len);

		//drv->send(buf); // This is for further studies

	}
}

static void net_rx_fiber(void)
{
	struct net_buf *buf;

	NET_DBG("\n");

	while (1) {
		/* Wait next packet from network, pass it to IP stack */
		buf = nano_fifo_get_wait(&netdev.rx_queue);

		net_buf_put(buf);
	}
}

static void init_rx_queue(void)
{
	nano_fifo_init(&netdev.rx_queue);

	fiber_start(rx_fiber_stack, STACKSIZE,
		    (nano_fiber_entry_t) net_rx_fiber, 0, 0, 7, 0);
}

static void init_tx_queue(void)
{
	nano_fifo_init(&netdev.tx_queue);

	fiber_start(tx_fiber_stack, STACKSIZE,
		    (nano_fiber_entry_t) net_tx_fiber, 0, 0, 7, 0);
}

static int network_initialization(void)
{
	netstack_init();
	return 0;
}

int net_register_driver(struct net_driver *drv)
{
	if (netdev.drv) {
		return -EALREADY;
	}

	if (!drv->open || !drv->send) {
		return -EINVAL;
	}

	netdev.drv = drv;

	return 0;
}

void net_unregister_driver(struct net_driver *drv)
{
	netdev.drv = NULL;
}

int net_init(void)
{
	struct net_driver *drv = netdev.drv;
	int err;

	if (!drv) {
		NET_DBG("No network driver defined\n");
		return -ENODEV;
	}

	net_buf_init();
	init_tx_queue();
	init_rx_queue();

	err = drv->open();
	if (err) {
		return err;
	}

	return network_initialization();
}
