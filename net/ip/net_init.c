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
#include <net/net_ip.h>
#include <net/net_socket.h>

#include "contiki/os/sys/process.h"
#include "contiki/os/sys/etimer.h"
#include "contiki/netstack.h"
#include "contiki/uip-driver.h"
#include "contiki/ipv6/uip-ds6.h"
#include "contiki/ip/simple-udp.h"

/* Declare some private functions only to be used in this file so the
 * prototypes are not found in .h file.
 */
struct nano_fifo *net_context_get_queue(struct net_context *context);
struct simple_udp_connection *
	net_context_get_udp_connection(struct net_context *context);
int net_context_get_receiver_registered(struct net_context *context);
void net_context_set_receiver_registered(struct net_context *context);

/* Stacks for the tx & rx fibers.
 * FIXME: stack size needs fine-tuning
 */
#define STACKSIZE 2048
static char rx_fiber_stack[STACKSIZE];
static char tx_fiber_stack[STACKSIZE];

static struct net_dev {
	/* Queue for incoming packets from driver */
	struct nano_fifo rx_queue;

	/* Queue for outgoing packets from apps */
	struct nano_fifo tx_queue;

	/* Registered network driver, FIXME: how this is set? */
	struct net_driver *drv;
} netdev;

/* Called by application to send a packet */
int net_send(struct net_buf *buf)
{
	if (buf->len == 0) {
		return -ENODATA;
	}

	nano_fifo_put(&netdev.tx_queue, buf);

	return 0;
}

/* Called by driver when an IP packet has been received */
int net_recv(struct net_buf *buf)
{
	if (buf->len == 0) {
		return -ENODATA;
	}

	nano_fifo_put(&netdev.rx_queue, buf);

	return 0;
}

static void udp_packet_receive(struct simple_udp_connection *c,
			       const uip_ipaddr_t *source_addr,
			       uint16_t source_port,
			       const uip_ipaddr_t *dest_addr,
			       uint16_t dest_port,
			       const uint8_t *data, uint16_t datalen,
			       void *user_data,
			       struct net_buf *buf)
{
	struct net_context *context = user_data;

	if (!context) {
		return;
	}

	uip_appdatalen(buf) = datalen;

	NET_DBG("packet received buf %p context %p len %d\n",
		buf, context, datalen);

	nano_fifo_put(net_context_get_queue(context), buf);
}

/* Called by application when it wants to receive network data */
struct net_buf *net_receive(struct net_context *context)
{
	struct nano_fifo *rx_queue = net_context_get_queue(context);
	struct net_tuple *tuple;
	struct simple_udp_connection *udp;
	int ret = 0;

	tuple = net_context_get_tuple(context);
	if (!tuple) {
		return NULL;
	}

	switch (tuple->ip_proto) {
	case IPPROTO_UDP:
		udp = net_context_get_udp_connection(context);
		if (!net_context_get_receiver_registered(context)) {
			ret = simple_udp_register(udp, tuple->local_port,
					  (uip_ip6addr_t *)tuple->remote_addr,
					  tuple->remote_port,
					  udp_packet_receive,
					  context);
			if (!ret) {
				NET_DBG("UDP connection listener failed\n");
				ret = -ENOENT;
				break;
			}
		}
		net_context_set_receiver_registered(context);
		ret = 0;
		break;
	case IPPROTO_TCP:
		NET_DBG("TCP not yet supported\n");
		ret = -EINVAL;
		break;
	case IPPROTO_ICMPV6:
		NET_DBG("ICMPv6 not yet supported\n");
		ret = -EINVAL;
		break;
	}

	return nano_fifo_get(rx_queue);
}

static void udp_packet_reply(struct simple_udp_connection *c,
			     const uip_ipaddr_t *source_addr,
			     uint16_t source_port,
			     const uip_ipaddr_t *dest_addr,
			     uint16_t dest_port,
			     const uint8_t *data, uint16_t datalen,
			     void *user_data,
			     struct net_buf *not_used)
{
	struct net_buf *buf = user_data;
	struct nano_fifo *queue;

	if (!buf->context) {
		return;
	}

	NET_DBG("packet reply buf %p context %p len %d\n", buf, buf->context,
		buf->len);

	queue = net_context_get_queue(buf->context);

	nano_fifo_put(queue, buf);
}

/* Internal function to send network data to uIP stack */
static int check_and_send_packet(struct net_buf *buf)
{
	struct net_tuple *tuple;
	struct simple_udp_connection *udp;
	int ret = 0;

	if (!netdev.drv) {
		return -EINVAL;
	}

	tuple = net_context_get_tuple(buf->context);
	if (!tuple) {
		return -EINVAL;
	}

	switch (tuple->ip_proto) {
	case IPPROTO_UDP:
		udp = net_context_get_udp_connection(buf->context);
		ret = simple_udp_register(udp, tuple->local_port,
					  (uip_ip6addr_t *)tuple->remote_addr,
					  tuple->remote_port,
					  udp_packet_reply,
					  buf);
		if (!ret) {
			NET_DBG("UDP connection creation failed\n");
			ret = -ENOENT;
			break;
		}
		simple_udp_send(buf, udp, buf->data, buf->len);
		ret = 0;
		break;
	case IPPROTO_TCP:
		NET_DBG("TCP not yet supported\n");
		ret = -EINVAL;
		break;
	case IPPROTO_ICMPV6:
		NET_DBG("ICMPv6 not yet supported\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}

static void net_tx_fiber(void)
{
	NET_DBG("Starting TX fiber\n");

	while (1) {
		struct net_buf *buf;
		uint8_t run;

		/* Get next packet from application - wait if necessary */
		buf = nano_fifo_get_wait(&netdev.tx_queue);

		NET_DBG("Sending (buf %p, len %u) to IP stack\n",
			buf, buf->len);

		if (check_and_send_packet(buf) < 0) {
			/* Release buffer on error */
			net_buf_put(buf);
			continue;
		}

		/* Check for any events that we might need to process */
		do {
			run = process_run(buf);
		} while (run > 0);
	}
}

static void net_rx_fiber(void)
{
	struct net_buf *buf;

	NET_DBG("Starting RX fiber\n");

	while (1) {
		/* Wait next packet from network */
		buf = nano_fifo_get_wait(&netdev.rx_queue);

		/* TBD: Pass it to IP stack */

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

int net_set_mac(uint8_t *mac, uint8_t len)
{
	uip_ds6_addr_t *lladdr;

	if ((len > UIP_LLADDR_LEN) || (len != 6 && len != 8)) {
		NET_ERR("Wrong ll addr len, len %d, max %d\n",
			len, UIP_LLADDR_LEN);
		return -EINVAL;
	}

	linkaddr_set_node_addr((linkaddr_t *)mac);
	uip_ds6_set_lladdr((uip_lladdr_t *)mac);

	lladdr = uip_ds6_get_link_local(-1);

	NET_DBG("Tentative link-local IPv6 address ");
	PRINT6ADDR(&lladdr->ipaddr);
	PRINTF("\n");

	lladdr->state = ADDR_AUTOCONF;

	return 0;
}

static uint8_t net_tcpip_output(struct net_buf *buf, const uip_lladdr_t *lladdr)
{
	if (!netdev.drv) {
		return 0;
	}

	if (netdev.drv->send(buf) < 0) {
		return 0;
	}

	return 1;
}

static int network_initialization(void)
{
	/* Initialize and start Contiki uIP stack */
	clock_init();

	rtimer_init();
	ctimer_init();

	process_init();
	tcpip_set_outputfunc(net_tcpip_output);

	process_start(&tcpip_process, NULL);
	process_start(&simple_udp_process, NULL);
	process_start(&etimer_process, NULL);

	return 0;
}

int net_register_driver(struct net_driver *drv)
{
	int r;

	if (netdev.drv) {
		return -EALREADY;
	}

	if (!drv->open || !drv->send) {
		return -EINVAL;
	}

	r = drv->open();
	if (r < 0) {
		return r;
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
	net_context_init();
	net_buf_init();
	init_tx_queue();
	init_rx_queue();

	return network_initialization();
}
