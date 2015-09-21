/** @file
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

#include <net/net_buf.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_socket.h>

#include "net_driver_15_4.h"
#include "net_driver_slip.h"

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
static char __noinit timer_fiber_stack[STACKSIZE_UNIT * 1];

static struct net_dev {
	/* Queue for incoming packets from driver */
	struct nano_fifo rx_queue;

	/* Queue for outgoing packets from apps */
	struct nano_fifo tx_queue;

	/* Registered network driver */
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

#ifdef CONFIG_NETWORKING_STATISTICS
#define STAT(s) uip_stat.s
#define PRINT_STATISTICS_INTERVAL (10 * sys_clock_ticks_per_sec)
#define net_print_statistics stats /* to make the debug print line shorter */

#if RPL_CONF_STATS
#include "rpl/rpl-private.h"
#endif

static void stats(void)
{
	static clock_time_t last_print;

	/* See contiki/ip/uip.h for descriptions of the different values */
	if (clock_time() > (last_print + PRINT_STATISTICS_INTERVAL)) {
		NET_DBG("IP recv\t%d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
			STAT(ip.recv),
			STAT(ip.sent),
			STAT(ip.drop),
			STAT(ip.forwarded));
		NET_DBG("IP vhlerr\t%d\thblener\t%d\tlblener\t%d\n",
			STAT(ip.vhlerr),
			STAT(ip.hblenerr),
			STAT(ip.lblenerr));
		NET_DBG("IP fragerr\t%d\tchkerr\t%d\tprotoer\t%d\n",
			STAT(ip.fragerr),
			STAT(ip.chkerr),
			STAT(ip.protoerr));

		NET_DBG("ICMP recv\t%d\tsent\t%d\tdrop\t%d\n",
			STAT(icmp.recv),
			STAT(icmp.sent),
			STAT(icmp.drop));
		NET_DBG("ICMP typeer\t%d\tchkerr\t%d\n",
			STAT(icmp.typeerr),
			STAT(icmp.chkerr));

		NET_DBG("UDP recv\t%d\tsent\t%d\tdrop\t%d\n",
			STAT(udp.recv),
			STAT(udp.sent),
			STAT(udp.drop));
		NET_DBG("UDP chkerr\t%d\n",
			STAT(icmp.chkerr));

#if NETSTACK_CONF_WITH_IPV6
		NET_DBG("ND recv\t%d\tsent\t%d\tdrop\t%d\n",
			STAT(nd6.recv),
			STAT(nd6.sent),
			STAT(nd6.drop));
#endif

#if RPL_CONF_STATS
#define RSTAT(s) RPL_STAT(rpl_stats.s)
		NET_DBG("RPL overflows\t%d\tl-repairs\t%d\tg-repairs\t%d\n",
			RSTAT(mem_overflows),
			RSTAT(local_repairs),
			RSTAT(global_repairs));
		NET_DBG("RPL malformed\t%d\tresets   \t%d\tp-switch\t%d\n",
			RSTAT(malformed_msgs),
			RSTAT(resets),
			RSTAT(parent_switch));
		NET_DBG("RPL f-errors\t%d\tl-errors\t%d\tl-warnings\t%d\n",
			RSTAT(forward_errors),
			RSTAT(loop_errors),
			RSTAT(loop_warnings));
		NET_DBG("RPL r-repairs\t%d\n",
			RSTAT(root_repairs));
#endif
		last_print = clock_time();
	}
}
#else
#define net_print_statistics()
#endif

/* Switch the ports and addresses and set route and neighbor cache.
 * Returns 1 if packet was sent properly, in this case it is the caller
 * that needs to release the net_buf. If 0 is returned, then uIP stack
 * has released the net_buf already because there was an some net related
 * error when sending the buffer.
 */
static inline int udp_prepare_and_send(struct net_context *context,
				       struct net_buf *buf)
{
#ifdef CONFIG_NETWORKING_IPV6_NO_ND
	uip_ds6_route_t *route_old, *route_new = NULL;
	uip_ds6_nbr_t *nbr;
#endif
	uip_ipaddr_t tmp;
	uint16_t port;
	uint8_t ret;

	if (uip_len(buf) == 0) {
		/* This is expected as uIP will typically set the
		 * packet length to 0 after receiving it. So we need
		 * to fix the length here. The protocol specific
		 * part is added also here.
		 */
		uip_len(buf) = uip_slen(buf) = uip_appdatalen(buf) =
			net_buf_datalen(buf);
	}

	port = NET_BUF_UDP(buf)->srcport;
	NET_BUF_UDP(buf)->srcport = NET_BUF_UDP(buf)->destport;
	NET_BUF_UDP(buf)->destport = port;

	uip_ipaddr_copy(&tmp, &NET_BUF_IP(buf)->srcipaddr);
	uip_ipaddr_copy(&NET_BUF_IP(buf)->srcipaddr,
			&NET_BUF_IP(buf)->destipaddr);
	uip_ipaddr_copy(&NET_BUF_IP(buf)->destipaddr, &tmp);

#ifdef CONFIG_NETWORKING_IPV6_NO_ND
	/* The peer needs to be in neighbor cache before route can be added.
	 */
	nbr = uip_ds6_nbr_lookup((uip_ipaddr_t *)&NET_BUF_IP(buf)->destipaddr);
	if (!nbr) {
		const uip_lladdr_t *lladdr = (const uip_lladdr_t *)&buf->src;
		nbr = uip_ds6_nbr_add(
			(uip_ipaddr_t *)&NET_BUF_IP(buf)->destipaddr,
			lladdr, 0, NBR_REACHABLE);
		if (!nbr) {
			NET_DBG("Cannot add peer ");
			PRINT6ADDR(&NET_BUF_IP(buf)->destipaddr);
			PRINT(" to neighbor cache\n");
		}
	}

	/* Temporarily add route to peer, delete the route after
	 * sending the packet. Check if there was already a
	 * route and do not remove it if there was existing
	 * route to this peer.
	 */
	route_old = uip_ds6_route_lookup(&NET_BUF_IP(buf)->destipaddr);
	if (!route_old) {
		route_new = uip_ds6_route_add(&NET_BUF_IP(buf)->destipaddr,
					      128,
					      &NET_BUF_IP(buf)->destipaddr);
		if (!route_new) {
			NET_DBG("Cannot add route to peer ");
			PRINT6ADDR(&NET_BUF_IP(buf)->destipaddr);
			PRINT("\n");
		}
	}
#endif

	ret = simple_udp_sendto_port(buf,
				     net_context_get_udp_connection(context),
				     buf->data, buf->len,
				     &NET_BUF_IP(buf)->destipaddr,
				     uip_ntohs(NET_BUF_UDP(buf)->destport));
	if (!ret) {
		NET_DBG("Packet could not be sent properly.\n");
	}

#ifdef CONFIG_NETWORKING_IPV6_NO_ND
	if (!route_old && route_new) {
		/* This will also remove the neighbor cache entry */
		uip_ds6_route_rm(route_new);
	}
#endif

	return !ret;
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
		/* If the context is not there, then we must discard
		 * the buffer here, otherwise we have a buffer leak.
		 */
		net_buf_put(buf);
		return;
	}

	uip_appdatalen(buf) = datalen;
	buf->datalen = datalen;
	buf->data = uip_appdata(buf);

	NET_DBG("packet received buf %p context %p len %d appdatalen %d\n",
		buf, context, buf->len, datalen);

	nano_fifo_put(net_context_get_queue(context), buf);
}

/* Called by application when it wants to receive network data */
struct net_buf *net_receive(struct net_context *context, int32_t timeout)
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
#ifdef CONFIG_NETWORKING_WITH_IPV6
				(uip_ip6addr_t *)&tuple->remote_addr->in6_addr,
#else
				(uip_ip4addr_t *)&tuple->remote_addr->in_addr,
#endif
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
	default:
		NET_ERR("Invalid IP protocol. "
			"Internal data structure corrupted!\n");
		ret = -EINVAL;
		break;
	}

	if (ret) {
		return NULL;
	}

	switch (timeout) {
	case TICKS_UNLIMITED:
		return nano_fifo_get_wait(rx_queue);
	case TICKS_NONE:
		return nano_fifo_get(rx_queue);
	default:
#ifdef CONFIG_NANO_TIMEOUTS
#ifdef CONFIG_MICROKERNEL
		return nano_task_fifo_get_wait_timeout(rx_queue, timeout);
#else /* CONFIG_MICROKERNEL */
		return nano_fiber_fifo_get_wait_timeout(rx_queue, timeout);
#endif
#else /* CONFIG_NANO_TIMEOUTS */
		return nano_fifo_get(rx_queue);
#endif
	}
}

static void udp_packet_reply(struct simple_udp_connection *c,
			     const uip_ipaddr_t *source_addr,
			     uint16_t source_port,
			     const uip_ipaddr_t *dest_addr,
			     uint16_t dest_port,
			     const uint8_t *data, uint16_t datalen,
			     void *user_data,
			     struct net_buf *buf)
{
	struct net_context *context = user_data;
	struct nano_fifo *queue;

	if (!context) {
		/* If the context is not there, then we must discard
		 * the buffer here, otherwise we have a buffer leak.
		 */
		net_buf_put(buf);
		return;
	}

	queue = net_context_get_queue(context);

	NET_DBG("packet reply buf %p context %p len %d queue %p\n",
		buf, context, buf->len, queue);

	/* Contiki stack will overwrite the uip_len(buf) and
	 * uip_appdatalen(buf) values, so in order to allow
	 * the application to use them, copy the values here.
	 */
	buf->datalen = uip_len(buf);
	buf->data = uip_appdata(buf);

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
#ifdef CONFIG_NETWORKING_WITH_IPV6
				(uip_ip6addr_t *)&tuple->remote_addr->in6_addr,
#else
				(uip_ip4addr_t *)&tuple->remote_addr->in_addr,
#endif
				tuple->remote_port, udp_packet_reply,
				buf->context);
			if (!ret) {
				NET_DBG("UDP connection creation failed\n");
				ret = -ENOENT;
				break;
			}
			net_context_set_receiver_registered(buf->context);
		}
		ret = simple_udp_send(buf, udp, buf->data, buf->len);
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
		uint8_t ret;

		/* Get next packet from application - wait if necessary */
		buf = nano_fifo_get_wait(&netdev.tx_queue);

		NET_DBG("Sending (buf %p, len %u) to IP stack\n",
			buf, buf->len);

		/* What to do with the buffer:
		 *  <0: error, release the buffer
		 *   0: message was discarded by uIP, release the buffer here
		 *  >0: message was sent ok, buffer released already
		 */
		ret = check_and_send_packet(buf);
		if (ret < 0) {
			net_buf_put(buf);
			continue;
		} else if (ret > 0) {
			continue;
		}

		NET_BUF_CHECK_IF_NOT_IN_USE(buf);

		/* Check for any events that we might need to process */
		do {
			ret = process_run(buf);
		} while (ret > 0);

		/* Check stack usage (no-op if not enabled) */
		net_analyze_stack("TX fiber", tx_fiber_stack,
				  sizeof(tx_fiber_stack));

		net_buf_put(buf);

		net_print_statistics();
	}
}

static void net_rx_fiber(void)
{
	struct net_buf *buf;

	NET_DBG("Starting RX fiber\n");

	while (1) {
		buf = nano_fifo_get_wait(&netdev.rx_queue);

		/* Check stack usage (no-op if not enabled) */
		net_analyze_stack("RX fiber", rx_fiber_stack,
				  sizeof(rx_fiber_stack));

		NET_DBG("Received buf %p\n", buf);

		if (!tcpip_input(buf)) {
			net_buf_put(buf);
		}
		/* The buffer is on to its way to receiver at this
		 * point. We must not remove it here.
		 */

		net_print_statistics();
	}
}

/*
 * Run various Contiki timers. At the moment this is done via polling.
 * Max. timeout is set to 60sec so that we wake up at least once a minute.
 */
#define DEFAULT_TIMER_WAKEUP (2 * sys_clock_ticks_per_sec)
#define MAX_TIMER_WAKEUP (60 * sys_clock_ticks_per_sec)

static void net_timer_fiber(void)
{
	clock_time_t next_wakeup;

	NET_DBG("Starting net timer fiber\n");

	while (1) {
		/* Run various timers */
		next_wakeup = etimer_request_poll();

		if (next_wakeup == 0) {
			/* There was no timers, wait a bit */
			next_wakeup = DEFAULT_TIMER_WAKEUP;
		} else {
			if (next_wakeup > MAX_TIMER_WAKEUP) {
				NET_DBG("Too long wakeup %d\n", next_wakeup);
				next_wakeup = MAX_TIMER_WAKEUP;
			}
			if (!(next_wakeup < CLOCK_SECOND * 2)) {
				NET_DBG("Next wakeup %d\n", next_wakeup);
			}

#ifdef CONFIG_INIT_STACKS
			{
				static clock_time_t last_print;
				if ((last_print + 10) < clock_seconds()) {
					net_analyze_stack("timer fiber",
							  timer_fiber_stack,
						  sizeof(timer_fiber_stack));
					last_print = clock_seconds();
				}
			}
#endif
		}

		fiber_sleep(next_wakeup);
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

static void init_timer_fiber(void)
{
	fiber_start(timer_fiber_stack, sizeof(timer_fiber_stack),
		    (nano_fiber_entry_t) net_timer_fiber, 0, 0, 7, 0);
}

int net_set_mac(uint8_t *mac, uint8_t len)
{
	if ((len > UIP_LLADDR_LEN) || (len != 6 && len != 8)) {
		NET_ERR("Wrong ll addr len, len %d, max %d\n",
			len, UIP_LLADDR_LEN);
		return -EINVAL;
	}

	linkaddr_set_node_addr((linkaddr_t *)mac);

#ifdef CONFIG_NETWORKING_WITH_IPV6
	{
		uip_ds6_addr_t *lladdr;

		uip_ds6_set_lladdr((uip_lladdr_t *)mac);

		lladdr = uip_ds6_get_link_local(-1);

		NET_DBG("Tentative link-local IPv6 address ");
		PRINT6ADDR(&lladdr->ipaddr);
		PRINTF("\n");

		lladdr->state = ADDR_AUTOCONF;
	}
#endif
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

	return netdev.drv->send(buf);
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
	init_timer_fiber();

#if defined (CONFIG_NETWORKING_WITH_15_4)
	net_driver_15_4_init();
#endif

	net_driver_slip_init();

	return network_initialization();
}
