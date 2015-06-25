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
#include <sections.h>
#include <string.h>
#include <errno.h>

#include <net/net_core.h>
#include <net/net_buf.h>
#include <net/net_ip.h>
#include <net/net_socket.h>

#include "net_driver_15_4.h"

#include "contiki/os/sys/process.h"
#include "contiki/os/sys/etimer.h"
#include "contiki/netstack.h"
#include "contiki/ipv6/uip-ds6.h"
#include "contiki/ip/simple-udp.h"
#include "contiki/os/dev/slip.h"

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
#define STACKSIZE_UNIT 1024
static char __noinit rx_fiber_stack[STACKSIZE_UNIT * 1];
static char __noinit tx_fiber_stack[STACKSIZE_UNIT * 1];

static struct net_dev {
	/* Queue for incoming packets from driver */
	struct nano_fifo rx_queue;

	/* Queue for outgoing packets from apps */
	struct nano_fifo tx_queue;

	/* Registered network driver, FIXME: how this is set? */
	struct net_driver *drv;
} netdev;

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_PRINTK)
#include <offsets.h>
#include <misc/printk.h>

enum {
	STACK_DIRECTION_UP,
	STACK_DIRECTION_DOWN,
};

static unsigned calculate_unused(const char *stack, unsigned size,
				 int stack_growth)
{
	unsigned i, unused = 0;

	if (stack_growth == STACK_DIRECTION_DOWN) {
		for (i = __tCCS_SIZEOF; i < size; i++) {
			if ((unsigned char)stack[i] == 0xaa) {
				unused++;
			} else {
				break;
			}
		}
	} else {
		for (i = size - 1; i >= __tCCS_SIZEOF; i--) {
			if ((unsigned char)stack[i] == 0xaa) {
				unused++;
			} else {
				break;
			}
		}
	}

	return unused;
}

static void analyze_stacks(struct net_buf *buf, struct net_buf **ref)
{
	unsigned unused_rx, unused_tx;
	int stack_growth;
	char *dir;

	if (buf > *ref) {
		dir = "up";
		stack_growth = STACK_DIRECTION_UP;
	} else {
		dir = "down";
		stack_growth = STACK_DIRECTION_DOWN;
	}

	unused_rx = calculate_unused(rx_fiber_stack, sizeof(rx_fiber_stack),
				     stack_growth);
	unused_tx = calculate_unused(tx_fiber_stack, sizeof(tx_fiber_stack),
				     stack_growth);

	printk("net: ip: stack grows %s, sizeof(tCCS): %u  "
	       "rx stack(%p/%u): unused %u/%u  "
	       "tx stack(%p/%u): unused %u/%u\n",
	       dir, __tCCS_SIZEOF,
	       rx_fiber_stack, sizeof(rx_fiber_stack),
	       unused_rx, sizeof(rx_fiber_stack),
	       tx_fiber_stack, sizeof(tx_fiber_stack),
	       unused_tx, sizeof(tx_fiber_stack));
}
#else
#define analyze_stacks(...)
#endif

/* Called by application to send a packet */
int net_send(struct net_buf *buf)
{
	if (buf->len == 0) {
		return -ENODATA;
	}

	nano_fifo_put(&netdev.tx_queue, buf);

	return 0;
}

#define UIP_IP_BUF(buf)   ((struct uip_ip_hdr *)&uip_buf(buf)[UIP_LLH_LEN])
#define UIP_UDP_BUF(buf)  \
	((struct uip_udp_hdr *)&uip_buf(buf)[UIP_LLH_LEN + UIP_IPH_LEN])

/* Switch the ports and addresses and set route and neighbor cache.
 * Returns 1 if packet was sent properly, in this case it is the caller
 * that needs to release the net_buf. If 0 is returned, then uIP stack
 * has released the net_buf already because there was an some net related
 * error when sending the buffer.
 */
static inline int udp_prepare_and_send(struct net_context *context,
				       struct net_buf *buf)
{
	uip_ds6_route_t *route_old, *route_new = NULL;
	uip_ds6_nbr_t *nbr;
	uip_ipaddr_t tmp;
	uint16_t port;
	uint8_t ret;

	if (uip_len(buf) == 0) {
		/* This is expected as uIP will typically set the
		 * packet length to 0 after receiving it. So we need
		 * to fix the length here. The protocol specific
		 * part is added also here.
		 */
		uip_len(buf) = uip_slen(buf) = uip_appdatalen(buf);
		buf->data = buf->buf + UIP_IPUDPH_LEN;
	}

	port = UIP_UDP_BUF(buf)->srcport;
	UIP_UDP_BUF(buf)->srcport = UIP_UDP_BUF(buf)->destport;
	UIP_UDP_BUF(buf)->destport = port;

	uip_ipaddr_copy(&tmp, &UIP_IP_BUF(buf)->srcipaddr);
	uip_ipaddr_copy(&UIP_IP_BUF(buf)->srcipaddr,
			&UIP_IP_BUF(buf)->destipaddr);
	uip_ipaddr_copy(&UIP_IP_BUF(buf)->destipaddr, &tmp);

	/* The peer needs to be in neighbor cache before route can be added.
	 */
	nbr = uip_ds6_nbr_lookup((uip_ipaddr_t *)&UIP_IP_BUF(buf)->destipaddr);
	if (!nbr) {
		const uip_lladdr_t *lladdr = (const uip_lladdr_t *)&buf->src;
		nbr = uip_ds6_nbr_add(
			(uip_ipaddr_t *)&UIP_IP_BUF(buf)->destipaddr,
			lladdr, 0, NBR_REACHABLE);
		if (!nbr) {
			NET_DBG("Cannot add peer ");
			PRINT6ADDR(&UIP_IP_BUF(buf)->destipaddr);
			PRINT(" to neighbor cache\n");
		}
	}

	/* Temporarily add route to peer, delete the route after
	 * sending the packet. Check if there was already a
	 * route and do not remove it if there was existing
	 * route to this peer.
	 */
	route_old = uip_ds6_route_lookup(&UIP_IP_BUF(buf)->destipaddr);
	if (!route_old) {
		route_new = uip_ds6_route_add(&UIP_IP_BUF(buf)->destipaddr,
					      128,
					      &UIP_IP_BUF(buf)->destipaddr);
		if (!route_new) {
			NET_DBG("Cannot add route to peer ");
			PRINT6ADDR(&UIP_IP_BUF(buf)->destipaddr);
			PRINT("\n");
		}
	}

	ret = simple_udp_sendto_port(buf,
				     net_context_get_udp_connection(context),
				     buf->data, buf->len,
				     &UIP_IP_BUF(buf)->destipaddr,
				     uip_ntohs(UIP_UDP_BUF(buf)->destport));
	if (!ret) {
		NET_DBG("Packet could not be sent properly.\n");
	}

	if (!route_old && route_new) {
		/* This will also remove the neighbor cache entry */
		uip_ds6_route_rm(route_new);
	}

	return ret;
}

/* Application wants to send a reply */
int net_reply(struct net_context *context, struct net_buf *buf)
{
	struct net_tuple *tuple;
	struct uip_udp_conn *udp;
	int ret = 0;

	if (!context || !buf) {
		return -EINVAL;
	}

	tuple = net_context_get_tuple(context);
	if (!tuple) {
		return -ENOENT;
	}

	switch (tuple->ip_proto) {
	case IPPROTO_UDP:
		udp = uip_udp_conn(buf);
		if (!udp) {
			NET_ERR("UDP connection missing\n");
			return -ESRCH;
		}

		ret = udp_prepare_and_send(context, buf);
		break;
	case IPPROTO_TCP:
		NET_DBG("TCP not yet supported\n");
		return -EINVAL;
		break;
	case IPPROTO_ICMPV6:
		NET_DBG("ICMPv6 not yet supported\n");
		return -EINVAL;
		break;
	}

	return ret;
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
	int ret = 0;

	tuple = net_context_get_tuple(context);
	if (!tuple) {
		return NULL;
	}

	switch (tuple->ip_proto) {
	case IPPROTO_UDP:
		if (!net_context_get_receiver_registered(context)) {
			struct simple_udp_connection *udp =
				net_context_get_udp_connection(context);

			ret = simple_udp_register(udp, tuple->local_port,
				(uip_ip6addr_t *)&tuple->remote_addr->in6_addr,
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
		if (!net_context_get_receiver_registered(buf->context)) {
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
			net_context_set_receiver_registered(buf->context);
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

		/* Check stack usage (no-op if not enabled) */
		analyze_stacks(buf, &buf);
	}
}

static void net_rx_fiber(void)
{
	struct net_buf *buf;

	NET_DBG("Starting RX fiber\n");

	while (1) {
		buf = nano_fifo_get_wait(&netdev.rx_queue);

		/* Check stack usage (no-op if not enabled) */
		analyze_stacks(buf, &buf);

		if (!tcpip_input(buf)) {
			net_buf_put(buf);
		}
	}
}

static void init_rx_queue(void)
{
	nano_fifo_init(&netdev.rx_queue);

	fiber_start(rx_fiber_stack, sizeof(rx_fiber_stack),
		    (nano_fiber_entry_t) net_rx_fiber, 0, 0, 7, 0);
}

static void init_tx_queue(void)
{
	nano_fifo_init(&netdev.tx_queue);

	fiber_start(tx_fiber_stack, sizeof(tx_fiber_stack),
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

	if(lladdr == NULL) {
		linkaddr_copy(&buf->dest, &linkaddr_null);
	} else {
		linkaddr_copy(&buf->dest, (const linkaddr_t *)lladdr);
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

	slip_start();

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
	static uint8_t initialized;

	if (initialized)
		return -EALREADY;

	initialized = 1;

	net_context_init();
	net_buf_init();
	init_tx_queue();
	init_rx_queue();

#if defined (CONFIG_NETWORKING_WITH_15_4)
	net_driver_15_4_init();
#endif

	return network_initialization();
}
