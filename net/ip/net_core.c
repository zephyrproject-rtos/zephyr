/** @file
 * @brief Network initialization
 *
 * Initialize the network IP stack. Create two fibers, one for reading data
 * from applications (Tx fiber) and one for reading data from IP stack
 * and passing that data to applications (Rx fiber).
 */

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

#ifdef CONFIG_NETWORKING_WITH_LOGGING
#define DEBUG 1
#endif
#include "contiki/ip/uip-debug.h"

#include <nanokernel.h>
#include <toolchain.h>
#include <sections.h>
#include <string.h>
#include <errno.h>

#include <net/ip_buf.h>
#include <net/l2_buf.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_socket.h>

#include "net_driver_15_4.h"
#include "net_driver_slip.h"
#include "net_driver_ethernet.h"
#include "net_driver_bt.h"

#include "contiki/os/sys/process.h"
#include "contiki/os/sys/etimer.h"
#include "contiki/os/sys/ctimer.h"
#include "contiki/netstack.h"
#include "contiki/ipv6/uip-ds6.h"
#include "contiki/ip/simple-udp.h"
#include "contiki/os/dev/slip.h"

#ifdef CONFIG_15_4_BEACON_SUPPORT
#include "contiki/mac/handler-802154.h"
#endif

#ifdef CONFIG_DHCP
#include "contiki/ip/dhcpc.h"
#endif

/* Declare some private functions only to be used in this file so the
 * prototypes are not found in .h file.
 */
struct nano_fifo *net_context_get_queue(struct net_context *context);
struct simple_udp_connection *
	net_context_get_udp_connection(struct net_context *context);
int net_context_get_receiver_registered(struct net_context *context);
void net_context_set_receiver_registered(struct net_context *context);
int net_context_tcp_init(struct net_context *context, struct net_buf *buf,
			 enum net_tcp_type);
int net_context_tcp_send(struct net_buf *buf);
void *net_context_get_internal_connection(struct net_context *context);
struct net_buf *net_context_tcp_get_pending(struct net_context *context);
void net_context_tcp_set_pending(struct net_context *context,
				 struct net_buf *buf);
void net_context_set_connection_status(struct net_context *context,
				       int status);
void net_context_unset_receiver_registered(struct net_context *context);
extern void net_context_tcp_set_retry_count(struct net_context *context,
					    uint8_t count);
extern uint8_t net_context_tcp_get_retry_count(struct net_context *context);

/* Stacks for the tx & rx fibers.
 * FIXME: stack size needs fine-tuning
 */
#define STACKSIZE_UNIT 1024
#ifndef CONFIG_IP_RX_STACK_SIZE
#define CONFIG_IP_RX_STACK_SIZE (STACKSIZE_UNIT * 1)
#endif
#ifndef CONFIG_IP_TX_STACK_SIZE
#define CONFIG_IP_TX_STACK_SIZE (STACKSIZE_UNIT * 1)
#endif
#ifndef CONFIG_IP_TIMER_STACK_SIZE
#define CONFIG_IP_TIMER_STACK_SIZE (STACKSIZE_UNIT * 3 / 2)
#endif
static char __noinit __stack rx_fiber_stack[CONFIG_IP_RX_STACK_SIZE];
static char __noinit __stack tx_fiber_stack[CONFIG_IP_TX_STACK_SIZE];
static char __noinit __stack timer_fiber_stack[CONFIG_IP_TIMER_STACK_SIZE];
static nano_thread_id_t timer_fiber_id, tx_fiber_id;

static uint8_t initialized;

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
	int ret = 0;

	if (!buf || ip_buf_len(buf) == 0) {
		return -ENODATA;
	}

	if (buf->len && !uip_appdatalen(buf)) {
		uip_appdatalen(buf) = ip_buf_appdatalen(buf);
	}

	switch (sys_execution_context_type_get()) {
	case NANO_CTX_ISR:
		break;
	case NANO_CTX_FIBER:
		fiber_yield();
		break;
	case NANO_CTX_TASK:
#ifdef CONFIG_MICROKERNEL
		task_yield();
#endif
		break;
	}

#ifdef CONFIG_NETWORKING_WITH_TCP
#define MAX_TCP_RETRY_COUNT 3
	if (ip_buf_context(buf) &&
	    net_context_get_tuple(ip_buf_context(buf))->ip_proto ==
							IPPROTO_TCP) {
		struct uip_conn *conn;
		int status;
		uint8_t retry_count;

		net_context_tcp_init(ip_buf_context(buf), buf,
				     NET_TCP_TYPE_CLIENT);

		status = net_context_get_connection_status(
							ip_buf_context(buf));
		NET_DBG("context %p buf %p status %d\n",
			ip_buf_context(buf), buf, status);

		switch (status) {
		case EISCONN:
			/* User should be able to send new data now. */
			NET_DBG("Send new data buf %p ref %d\n", buf, buf->ref);
			net_context_set_connection_status(ip_buf_context(buf),
							  0);
			conn = (struct uip_conn *)net_context_get_internal_connection(ip_buf_context(buf));
			if (conn->buf) {
				ip_buf_unref(conn->buf);

				if (conn->buf == buf) {
					conn->buf = NULL;
					return 0;
				}

				conn->buf = NULL;
			}
			break;

		case -EALREADY:
			NET_DBG("Connection established\n");
			return 0;

		case -EINPROGRESS:
			NET_DBG("Connection being established\n");
			retry_count = net_context_tcp_get_retry_count(
							ip_buf_context(buf));
			if (retry_count < MAX_TCP_RETRY_COUNT) {
				net_context_tcp_set_retry_count(
					ip_buf_context(buf), ++retry_count);
				return status;
			}
			net_context_tcp_set_retry_count(ip_buf_context(buf),
							0);
			break;

		case -ECONNRESET:
			NET_DBG("Connection reset\n");
			net_context_unset_receiver_registered(
				ip_buf_context(buf));
			return status;
		}

		ret = status;
	}
#endif

	nano_fifo_put(&netdev.tx_queue, buf);

	/* Tell the IP stack it can proceed with the packet */
	fiber_wakeup(tx_fiber_id);

	return ret;
}

#ifdef CONFIG_NETWORKING_STATISTICS
#define STAT(s) uip_stat.s
#define PRINT_STATISTICS_INTERVAL (10 * sys_clock_ticks_per_sec)
#define net_print_statistics stats /* to make the debug print line shorter */

#if NET_MAC_CONF_STATS
#include "mac/mac.h"
#endif

#if RPL_CONF_STATS
#include "rpl/rpl-private.h"
#endif

#if NET_COAP_CONF_STATS
#include "er-coap/er-coap.h"
#endif

#if HANDLER_802154_CONF_STATS
#include "mac/handler-802154.h"
#endif

static void stats(void)
{
	static clock_time_t last_print;

	/* See contiki/ip/uip.h for descriptions of the different values */
	if (clock_time() > (last_print + PRINT_STATISTICS_INTERVAL)) {
#if NET_MAC_CONF_STATS
#define MAC_STAT(s) (net_mac_stats.s)
		NET_DBG("L2 bytes recv  %d\tsent\t%d\n",
			MAC_STAT(bytes_received),
			MAC_STAT(bytes_sent));
#endif
		NET_DBG("IP recv        %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
			STAT(ip.recv),
			STAT(ip.sent),
			STAT(ip.drop),
			STAT(ip.forwarded));
		NET_DBG("IP vhlerr      %d\thblener\t%d\tlblener\t%d\n",
			STAT(ip.vhlerr),
			STAT(ip.hblenerr),
			STAT(ip.lblenerr));
		NET_DBG("IP fragerr     %d\tchkerr\t%d\tprotoer\t%d\n",
			STAT(ip.fragerr),
			STAT(ip.chkerr),
			STAT(ip.protoerr));

#ifdef CONFIG_NETWORKING_WITH_TCP
		NET_DBG("TCP recv       %d\tsent\t%d\tdrop\t%d\n",
			STAT(tcp.recv),
			STAT(tcp.sent),
			STAT(tcp.drop));
		NET_DBG("TCP chkerr     %d\tackerr\t%d\trst\t%d\n",
			STAT(tcp.chkerr),
			STAT(tcp.ackerr),
			STAT(tcp.rst));
		NET_DBG("TCP rexmit     %d\tsyndrop\t%d\tsynrst\t%d\n",
			STAT(tcp.rexmit),
			STAT(tcp.syndrop),
			STAT(tcp.synrst));
#endif

		NET_DBG("ICMP recv      %d\tsent\t%d\tdrop\t%d\n",
			STAT(icmp.recv),
			STAT(icmp.sent),
			STAT(icmp.drop));
		NET_DBG("ICMP typeer    %d\tchkerr\t%d\n",
			STAT(icmp.typeerr),
			STAT(icmp.chkerr));

		NET_DBG("UDP recv       %d\tsent\t%d\tdrop\t%d\n",
			STAT(udp.recv),
			STAT(udp.sent),
			STAT(udp.drop));
		NET_DBG("UDP chkerr     %d\n",
			STAT(icmp.chkerr));

#if NET_COAP_CONF_STATS
		NET_DBG("CoAP recv      %d\terr\t%d\tsent\t%d\tre-sent\t%d\n",
			NET_COAP_STAT(recv),
			NET_COAP_STAT(recv_err),
			NET_COAP_STAT(sent),
			NET_COAP_STAT(re_sent));
#endif

#if NETSTACK_CONF_WITH_IPV6
		NET_DBG("ND recv        %d\tsent\t%d\tdrop\t%d\n",
			STAT(nd6.recv),
			STAT(nd6.sent),
			STAT(nd6.drop));
#endif

#if RPL_CONF_STATS
#define RSTAT(s) RPL_STAT(rpl_stats.s)
		NET_DBG("RPL overflows  %d\tl-repairs\t%d\tg-repairs\t%d\n",
			RSTAT(mem_overflows),
			RSTAT(local_repairs),
			RSTAT(global_repairs));
		NET_DBG("RPL malformed  %d\tresets   \t%d\tp-switch\t%d\n",
			RSTAT(malformed_msgs),
			RSTAT(resets),
			RSTAT(parent_switch));
		NET_DBG("RPL f-errors   %d\tl-errors\t%d\tl-warnings\t%d\n",
			RSTAT(forward_errors),
			RSTAT(loop_errors),
			RSTAT(loop_warnings));
		NET_DBG("RPL r-repairs  %d\n",
			RSTAT(root_repairs));
#endif

#if HANDLER_802154_CONF_STATS
#define IEEE802154_STAT(s) (handler_802154_stats.s)
		NET_DBG("802.15.4 beacons recv\t%d\tsent\t%d\treqs sent\t%d\n",
			IEEE802154_STAT(beacons_received),
			IEEE802154_STAT(beacons_sent),
			IEEE802154_STAT(beacons_reqs_sent));
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
		uip_len(buf) = ip_buf_len(buf);
	}

	ip_buf_appdata(buf) = &uip_buf(buf)[UIP_IPUDPH_LEN + UIP_LLH_LEN];

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
		const uip_lladdr_t *lladdr =
			(const uip_lladdr_t *)&ip_buf_ll_src(buf);
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
				     ip_buf_appdata(buf),
				     ip_buf_appdatalen(buf),
				     &NET_BUF_IP(buf)->destipaddr,
				     uip_ntohs(NET_BUF_UDP(buf)->destport));
	if (ret <= 0) {
		ret = -EINVAL;
		NET_DBG("Packet could not be sent properly.\n");
	} else {
		ret = 0;
	}

#ifdef CONFIG_NETWORKING_IPV6_NO_ND
	if (!route_old && route_new) {
		/* This will also remove the neighbor cache entry */
		uip_ds6_route_rm(route_new);
	}
#endif

	return ret;
}

#ifdef CONFIG_NETWORKING_WITH_TCP
/* Switch the ports and addresses. Returns 1 if packet was sent properly,
 * in this case it is the caller that needs to release the net_buf.
 * If 0 is returned, then uIP stack has released the net_buf already
 * because there was an some net related error when sending the buffer.
 */
static inline int tcp_prepare_and_send(struct net_context *context,
				       struct net_buf *buf)
{
#ifdef CONFIG_NETWORKING_IPV6_NO_ND
	uip_ds6_route_t *route_old, *route_new = NULL;
	uip_ds6_nbr_t *nbr;
#endif
	uip_ipaddr_t tmp;
	uint16_t port;
	int ret;

	uip_len(buf) = uip_slen(buf) = ip_buf_len(buf);
	uip_flags(buf) |= UIP_NEWDATA;
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
		const uip_lladdr_t *lladdr =
			(const uip_lladdr_t *)&ip_buf_ll_src(buf);
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

	NET_DBG("Packet output len %d\n", uip_len(buf));

	ret = net_context_tcp_send(buf);
	if (ret < 0 && ret != -EAGAIN) {
		NET_DBG("Packet could not be sent properly (err %d)\n", ret);
	}
	ip_buf_sent_status(buf) = 0;

#ifdef CONFIG_NETWORKING_IPV6_NO_ND
	if (!route_old && route_new) {
		/* This will also remove the neighbor cache entry */
		uip_ds6_route_rm(route_new);
	}
#endif

	return ret;
}
#endif

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

	switch (sys_execution_context_type_get()) {
	case NANO_CTX_ISR:
		break;
	case NANO_CTX_FIBER:
		fiber_yield();
		break;
	case NANO_CTX_TASK:
#ifdef CONFIG_MICROKERNEL
		task_yield();
#endif
		break;
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
#ifdef CONFIG_NETWORKING_WITH_TCP
		ret = tcp_prepare_and_send(context, buf);
#else
		NET_DBG("TCP not supported\n");
		return -EINVAL;
#endif
		break;
	case IPPROTO_ICMPV6:
		NET_DBG("ICMPv6 not yet supported\n");
		return -EINVAL;
	}

	return ret;
}

/* Called by driver when an IP packet has been received */
int net_recv(struct net_buf *buf)
{
	if (ip_buf_len(buf) == 0) {
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
		ip_buf_unref(buf);
		return;
	}

	ip_buf_appdatalen(buf) = datalen;
	ip_buf_appdata(buf) = &uip_buf(buf)[UIP_IPUDPH_LEN + UIP_LLH_LEN];
	ip_buf_len(buf) = datalen + UIP_IPUDPH_LEN + UIP_LLH_LEN;

	NET_DBG("packet received context %p len %d "
		"appdata %p appdatalen %d\n",
		context, ip_buf_len(buf),
		ip_buf_appdata(buf), ip_buf_appdatalen(buf));

	nano_fifo_put(net_context_get_queue(context), buf);
}

#ifdef CONFIG_NANO_TIMEOUTS
static inline struct net_buf *buf_wait_timeout(struct nano_fifo *queue,
					       int32_t timeout)
{
	switch (sys_execution_context_type_get()) {
	case NANO_CTX_FIBER:
		return nano_fiber_fifo_get(queue, timeout);
	case NANO_CTX_TASK:
		return nano_task_fifo_get(queue, timeout);
	case NANO_CTX_ISR:
	default:
		/* Invalid context type */
		break;
	}

	return NULL;
}
#endif

/* Called by application when it wants to receive network data */
struct net_buf *net_receive(struct net_context *context, int32_t timeout)
{
	struct nano_fifo *rx_queue = net_context_get_queue(context);
	struct net_buf *buf;
	struct net_tuple *tuple;
	int ret = 0;
	uint16_t reserve = 0;

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
		reserve = UIP_IPUDPH_LEN + UIP_LLH_LEN;
		break;
	case IPPROTO_TCP:
#ifdef CONFIG_NETWORKING_WITH_TCP
		ret = net_context_tcp_init(context, NULL, NET_TCP_TYPE_SERVER);
		if (ret) {
			NET_DBG("TCP connection init failed\n");
			ret = -ENOENT;
			break;
		}
		ret = 0;
#else
		NET_DBG("TCP not supported\n");
		ret = -EINVAL;
#endif
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
	case TICKS_NONE:
		buf = net_buf_get_timeout(rx_queue, 0, timeout);
		break;
	default:
#ifdef CONFIG_NANO_TIMEOUTS
		buf = buf_wait_timeout(rx_queue, timeout);
#else /* CONFIG_NANO_TIMEOUTS */
		buf = net_buf_get_timeout(rx_queue, 0, TICKS_NONE);
#endif
		break;
	}

#ifdef CONFIG_NETWORKING_WITH_TCP
	if (tuple->ip_proto == IPPROTO_TCP &&
	    (ip_buf_appdata(buf) > (void *)buf->data)) {
		/* We need to skip the TCP header + possible extensions */
		reserve = ip_buf_appdata(buf) - (void *)buf->data;
	}
#endif

	if (buf && reserve) {
		ip_buf_appdatalen(buf) = ip_buf_len(buf) - reserve;
		ip_buf_appdata(buf) = &uip_buf(buf)[reserve];
	}

	return buf;
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
		ip_buf_unref(buf);
		return;
	}

	queue = net_context_get_queue(context);

	/* Contiki stack will overwrite the uip_len(buf) and
	 * uip_appdatalen(buf) values, so in order to allow
	 * the application to use them, copy the values here.
	 */
	ip_buf_appdatalen(buf) = datalen;

	NET_DBG("packet reply context %p len %d "
		"appdata %p appdatalen %d queue %p\n",
		context, ip_buf_len(buf),
		ip_buf_appdata(buf), ip_buf_appdatalen(buf), queue);

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

	tuple = net_context_get_tuple(ip_buf_context(buf));
	if (!tuple) {
		return -EINVAL;
	}

	switch (tuple->ip_proto) {
	case IPPROTO_UDP:
		udp = net_context_get_udp_connection(ip_buf_context(buf));
		if (!net_context_get_receiver_registered(ip_buf_context(buf))) {
			ret = simple_udp_register(udp, tuple->local_port,
#ifdef CONFIG_NETWORKING_WITH_IPV6
				(uip_ip6addr_t *)&tuple->remote_addr->in6_addr,
#else
				(uip_ip4addr_t *)&tuple->remote_addr->in_addr,
#endif
				tuple->remote_port, udp_packet_reply,
				ip_buf_context(buf));
			if (!ret) {
				NET_DBG("UDP connection creation failed\n");
				ret = -ENOENT;
				break;
			}
			net_context_set_receiver_registered(ip_buf_context(buf));
		}

		if (ip_buf_appdatalen(buf) == 0) {
			/* User application has not set the application data
			 * length. The buffer will be discarded if we do not
			 * set the value correctly.
			 */
			uip_appdatalen(buf) = buf->len -
					      (UIP_IPUDPH_LEN + UIP_LLH_LEN);
		}

		ret = simple_udp_send(buf, udp, uip_appdata(buf),
				      uip_appdatalen(buf));
		break;
	case IPPROTO_TCP:
#ifdef CONFIG_NETWORKING_WITH_TCP
		if (ip_buf_appdatalen(buf) == 0) {
			/* User application has not set the application data
			 * length. The buffer will be discarded if we do not
			 * set the value correctly.
			 */
			uip_appdatalen(buf) = buf->len -
					      (UIP_IPTCPH_LEN + UIP_LLH_LEN);
		}
		if (uip_len(buf) == 0) {
			uip_len(buf) = buf->len;
		}
		ret = net_context_tcp_send(buf);
		if (ret < 0 && ret != -EAGAIN) {
			NET_DBG("Packet could not be sent properly "
				"(err %d)\n", ret);
		} else if (ret == 0) {
			/* For TCP the return status 0 means that the packet
			 * is released already. The caller of this function
			 * expects return value of > 0 in this case.
			 */
			ret = 1;
		} else {
			ip_buf_sent_status(buf) = ret;
			ret = true; /* This will prevent caller to discard
				     * the buffer that needs to be resent
				     * again.
				     */
		}
#else
		NET_DBG("TCP not supported\n");
		ret = -EINVAL;
#endif
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
	NET_DBG("Starting TX fiber (stack %zu bytes)\n",
		sizeof(tx_fiber_stack));

	while (1) {
		struct net_buf *buf;
		int ret;

		/* Get next packet from application - wait if necessary */
		buf = net_buf_get_timeout(&netdev.tx_queue, 0, TICKS_UNLIMITED);

		NET_DBG("Sending (buf %p, len %u) to IP stack\n",
			buf, buf->len);

		/* What to do with the buffer:
		 *  <0: error, release the buffer
		 *   0: message was discarded by uIP, release the buffer here
		 *  >0: message was sent ok, buffer released already
		 */
		ret = check_and_send_packet(buf);
		if (ret < 0) {
			ip_buf_unref(buf);
			goto wait_next;
		} else if (ret > 0) {
			goto wait_next;
		}

		NET_BUF_CHECK_IF_NOT_IN_USE(buf);

		/* Check for any events that we might need to process */
		do {
			ret = process_run(buf);
		} while (ret > 0);

		ip_buf_unref(buf);

	wait_next:
		/* Check stack usage (no-op if not enabled) */
		net_analyze_stack("TX fiber", tx_fiber_stack,
				  sizeof(tx_fiber_stack));

		net_print_statistics();
	}
}

static void net_rx_fiber(void)
{
	struct net_buf *buf;

	NET_DBG("Starting RX fiber (stack %zu bytes)\n",
		sizeof(rx_fiber_stack));

	while (1) {
		buf = net_buf_get_timeout(&netdev.rx_queue, 0, TICKS_UNLIMITED);

		/* Check stack usage (no-op if not enabled) */
		net_analyze_stack("RX fiber", rx_fiber_stack,
				  sizeof(rx_fiber_stack));

		NET_DBG("Received buf %p\n", buf);

		if (!tcpip_input(buf)) {
			ip_buf_unref(buf);
		}
		/* The buffer is on to its way to receiver at this
		 * point. We must not remove it here.
		 */

		net_print_statistics();
	}
}

/*
 * Run various Contiki timers.
 */
#define MAX_TIMER_WAKEUP 0x7ffffff

static void net_timer_fiber(void)
{
	clock_time_t next_wakeup;

	NET_DBG("Starting net timer fiber (stack %zu bytes)\n",
		sizeof(timer_fiber_stack));

	while (1) {
		/* Run various timers */
		next_wakeup = etimer_request_poll();

		if (next_wakeup == 0) {
			/* There was no timers, wait again */
			next_wakeup = MAX_TIMER_WAKEUP;
		} else {
			if (next_wakeup > MAX_TIMER_WAKEUP) {
				next_wakeup = MAX_TIMER_WAKEUP;
			}

#ifdef CONFIG_INIT_STACKS
			{
#define PRINT_CYCLE (60 * sys_clock_ticks_per_sec)

				static uint32_t next_print;
				uint32_t curr = sys_tick_get_32();

				/* Print stack usage every n. sec */
				if (!next_print ||
				    (next_print < curr &&
				     (!((curr - next_print) > PRINT_CYCLE)))) {
					uint32_t new_print;

					net_analyze_stack("timer fiber",
							  timer_fiber_stack,
						  sizeof(timer_fiber_stack));
					new_print = curr + PRINT_CYCLE;
					if (new_print > curr) {
						next_print = new_print;
					} else {
						/* Overflow */
						next_print = PRINT_CYCLE -
							(0xffffffff - curr);
					}
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
		    (nano_fiber_entry_t)net_rx_fiber, 0, 0, 7, 0);
}

static void init_tx_queue(void)
{
	nano_fifo_init(&netdev.tx_queue);

	tx_fiber_id = fiber_start(tx_fiber_stack, sizeof(tx_fiber_stack),
				  (nano_fiber_entry_t)net_tx_fiber,
				  0, 0, 7, 0);
}

static void init_timer_fiber(void)
{
	timer_fiber_id = fiber_start(timer_fiber_stack,
				     sizeof(timer_fiber_stack),
				     (nano_fiber_entry_t)net_timer_fiber,
				     0, 0, 7, 0);
}

void net_timer_check(void)
{
	fiber_wakeup(timer_fiber_id);
}

int net_set_mac(uint8_t *mac, uint8_t len)
{
	if (!mac) {
		NET_ERR("MAC address cannot be NULL\n");
		return -EINVAL;
	}

	if ((len > UIP_LLADDR_LEN) || (len != 6 && len != 8)) {
		NET_ERR("Wrong ll addr len, len %d, max %d\n",
			len, UIP_LLADDR_LEN);
		return -EINVAL;
	}

	linkaddr_set_node_addr((linkaddr_t *)mac);
	NET_DBG("MAC "); PRINTLLADDR((uip_lladdr_t *)&linkaddr_node_addr); PRINTF("\n");

#ifdef CONFIG_NETWORKING_WITH_IPV6
	if (!initialized) {
		memcpy(&uip_lladdr, mac, len);
	} else {
		uip_ds6_addr_t *lladdr;

		uip_ds6_set_lladdr((uip_lladdr_t *)mac);

		lladdr = uip_ds6_get_link_local(-1);

		NET_DBG("Tentative link-local IPv6 address ");
		PRINT6ADDR(&lladdr->ipaddr);
		PRINTF("\n");
	}
#else
	memcpy(&uip_lladdr, mac, len);

	NET_DBG("IPv4 address ");
	PRINT6ADDR(&uip_hostaddr);
	PRINTF("\n");
#endif
	return 0;
}

static uint8_t net_tcpip_output(struct net_buf *buf, const uip_lladdr_t *lladdr)
{
	int res;

	if (!netdev.drv) {
		return 0;
	}

	if (lladdr) {
		linkaddr_copy(&ip_buf_ll_dest(buf),
			      (const linkaddr_t *)lladdr);
	} else {
		linkaddr_copy(&ip_buf_ll_dest(buf), &linkaddr_null);
	}

	if (ip_buf_len(buf) == 0) {
		return 0;
	}

	res = netdev.drv->send(buf);
	if (res < 0) {
		res = 0;
	}
	return (uint8_t)res;
}

static int network_initialization(void)
{
	/* Initialize and start Contiki uIP stack */
	clock_init();

	rtimer_init();
	ctimer_init();

	process_init();
	tcpip_set_outputfunc(net_tcpip_output);

	process_start(&tcpip_process, NULL, NULL);
	process_start(&simple_udp_process, NULL, NULL);
	process_start(&etimer_process, NULL, NULL);
	process_start(&ctimer_process, NULL, NULL);

	slip_start();

#if CONFIG_15_4_BEACON_SUPPORT && CONFIG_NETWORKING_WITH_15_4_PAN_ID
	handler_802154_join(CONFIG_NETWORKING_WITH_15_4_PAN_ID, 1);
#endif

#ifdef CONFIG_NETWORKING_WITH_15_4_PAN_ID
	NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, IEEE802154_PANID);
#endif

#ifdef CONFIG_DHCP
	dhcpc_init(uip_lladdr.addr, sizeof(uip_lladdr.addr));
#endif
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
	if (initialized)
		return -EALREADY;

	initialized = 1;

#if UIP_STATISTICS == 1
	memset(&uip_stat, 0, sizeof(uip_stat));
#endif /* UIP_STATISTICS == 1 */

	net_context_init();

	ip_buf_init();
	l2_buf_init();

	init_tx_queue();
	init_rx_queue();
	init_timer_fiber();

#if defined(CONFIG_NETWORKING_WITH_15_4)
	net_driver_15_4_init();
#endif

#if defined(CONFIG_NETWORKING_WITH_BT)
	net_driver_bt_init();
#endif

	net_driver_slip_init();
	net_driver_ethernet_init();

	return network_initialization();
}
