/* net_driver_15_4.c - IP 15.4 driver */

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
#include <net/net_buf.h>
#include <net/net_ip.h>
#include <net/net_socket.h>
#include <net/netstack.h>
#include <net_driver_15_4.h>

/* Stacks for the tx & rx fibers.
 * FIXME: stack size needs fine-tuning
 */
#define STACKSIZE_UNIT 1024
static char __noinit rx_fiber_stack[STACKSIZE_UNIT * 1];
static char __noinit tx_fiber_stack[STACKSIZE_UNIT * 4];

/* Queue for incoming packets from hw driver */
static struct nano_fifo rx_queue;
/* Queue for outgoing packets to IP stack */
static struct nano_fifo tx_queue;

static int net_driver_15_4_open(void)
{
	return 0;
}

static int net_driver_15_4_send(struct net_buf *buf)
{
	int orig_len = buf->len;

	if (!NETSTACK_COMPRESS.compress(buf)) {
		NET_DBG("compression failed\n");
		net_buf_put(buf);
		return -EINVAL;
	}

	NET_DBG("sending %d bytes (original len %d)\n", buf->len, orig_len);

	nano_fifo_put(&tx_queue, buf);
	return 1;
}

static void net_tx_15_4_fiber(void)
{
	NET_DBG("Starting 15.4 TX fiber\n");

	while (1) {
		struct net_buf *buf;

		/* Get next packet from application - wait if necessary */
		buf = nano_fifo_get_wait(&tx_queue);

		if (uip_len(buf) == 0) {
			/* It is possible that uIP stack overwrote the len.
			 * We need to fix this here.
			 */
			uip_len(buf) = uip_slen(buf) = uip_appdatalen(buf) =
				net_buf_datalen(buf);
		}

		NET_DBG("Sending (buf %p, len %u) to 15.4 stack\n",
								buf, buf->len);

		if (!NETSTACK_FRAGMENT.fragment(buf, NULL)) {
			/* Release buffer on error */
			net_buf_put(buf);
		}

		net_analyze_stack("802.15.4 TX", tx_fiber_stack,
				  sizeof(tx_fiber_stack));
	}
}

static void net_rx_15_4_fiber(void)
{
	struct net_mbuf *buf;
#if NET_MAC_CONF_STATS
	int byte_count;
#endif

	NET_DBG("Starting 15.4 RX fiber\n");

	while (1) {
		/* Wait next packet from 15.4 stack */
		buf = nano_fifo_get_wait(&rx_queue);

#if NET_MAC_CONF_STATS
		byte_count = uip_pkt_buflen(buf);
#endif
		if (!NETSTACK_RDC.input(buf)) {
			NET_DBG("RDC input failed\n");
			net_mbuf_put(buf);
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

static void init_tx_queue(void)
{
	nano_fifo_init(&tx_queue);

	fiber_start(tx_fiber_stack, sizeof(tx_fiber_stack),
		    (nano_fiber_entry_t) net_tx_15_4_fiber, 0, 0, 7, 0);
}

static struct net_driver net_driver_15_4 = {
	.head_reserve = 0,
	.open = net_driver_15_4_open,
	.send = net_driver_15_4_send,
};

int net_driver_15_4_init(void)
{
	init_tx_queue();
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
	if (!NETSTACK_COMPRESS.uncompress(buf)) {
		NET_DBG("uncompression failed \n");
		return -EINVAL;
	}

	if (net_recv(buf) < 0) {
		NET_DBG("input to IP stack failed\n");
		return -EINVAL;
	}

	return 0;
}

int net_driver_15_4_recv_from_hw(struct net_mbuf *buf)
{
	nano_fifo_put(&rx_queue, buf);
	return 0;
}
