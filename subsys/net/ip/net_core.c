/** @file
 * @brief Network initialization
 *
 * Initialize the network IP stack. Create one thread for reading data
 * from IP stack and passing that data to applications (Rx thread).
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_core, CONFIG_NET_CORE_LOG_LEVEL);

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <string.h>
#include <errno.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/gptp.h>
#include <zephyr/net/websocket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/capture.h>

#if defined(CONFIG_NET_LLDP)
#include <zephyr/net/lldp.h>
#endif

#include "net_private.h"
#include "net_shell.h"

#include "icmpv6.h"
#include "ipv6.h"

#include "icmpv4.h"

#include "dhcpv4.h"

#include "route.h"

#include "packet_socket.h"
#include "canbus_socket.h"

#include "connection.h"
#include "udp_internal.h"
#include "tcp_internal.h"
#include "ipv4_autoconf_internal.h"

#include "net_stats.h"

static inline enum net_verdict process_data(struct net_pkt *pkt,
					    bool is_loopback)
{
	int ret;
	bool locally_routed = false;

	net_pkt_set_l2_processed(pkt, false);

	/* Initial call will forward packets to SOCK_RAW packet sockets. */
	ret = net_packet_socket_input(pkt, ETH_P_ALL);
	if (ret != NET_CONTINUE) {
		return ret;
	}

	/* If the packet is routed back to us when we have reassembled
	 * an IPv6 packet, then do not pass it to L2 as the packet does
	 * not have link layer headers in it.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV6_FRAGMENT) && net_pkt_ipv6_fragment_start(pkt)) {
		locally_routed = true;
	}

	/* If there is no data, then drop the packet. */
	if (!pkt->frags) {
		NET_DBG("Corrupted packet (frags %p)", pkt->frags);
		net_stats_update_processing_error(net_pkt_iface(pkt));

		return NET_DROP;
	}

	if (!is_loopback && !locally_routed) {
		ret = net_if_recv_data(net_pkt_iface(pkt), pkt);
		if (ret != NET_CONTINUE) {
			if (ret == NET_DROP) {
				NET_DBG("Packet %p discarded by L2", pkt);
				net_stats_update_processing_error(
							net_pkt_iface(pkt));
			}

			return ret;
		}
	}

	net_pkt_set_l2_processed(pkt, true);

	/* L2 has modified the buffer starting point, it is easier
	 * to re-initialize the cursor rather than updating it.
	 */
	net_pkt_cursor_init(pkt);

	if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET_DGRAM)) {
		/* Consecutive call will forward packets to SOCK_DGRAM packet sockets
		 * (after L2 removed header).
		 */
		ret = net_packet_socket_input(pkt, ETH_P_ALL);
		if (ret != NET_CONTINUE) {
			return ret;
		}
	}

	uint8_t family = net_pkt_family(pkt);

	if (IS_ENABLED(CONFIG_NET_IP) && (family == AF_INET || family == AF_INET6 ||
					  family == AF_UNSPEC || family == AF_PACKET)) {
		/* L2 processed, now we can pass IPPROTO_RAW to packet socket:
		 */
		ret = net_packet_socket_input(pkt, IPPROTO_RAW);
		if (ret != NET_CONTINUE) {
			return ret;
		}

		/* IP version and header length. */
		uint8_t vtc_vhl = NET_IPV6_HDR(pkt)->vtc & 0xf0;

		if (IS_ENABLED(CONFIG_NET_IPV6) && vtc_vhl == 0x60) {
			return net_ipv6_input(pkt, is_loopback);
		} else if (IS_ENABLED(CONFIG_NET_IPV4) && vtc_vhl == 0x40) {
			return net_ipv4_input(pkt);
		}

		NET_DBG("Unknown IP family packet (0x%x)", NET_IPV6_HDR(pkt)->vtc & 0xf0);
		net_stats_update_ip_errors_protoerr(net_pkt_iface(pkt));
		net_stats_update_ip_errors_vhlerr(net_pkt_iface(pkt));
		return NET_DROP;
	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_CAN) && family == AF_CAN) {
		return net_canbus_socket_input(pkt);
	}

	NET_DBG("Unknown protocol family packet (0x%x)", family);
	return NET_DROP;
}

static void processing_data(struct net_pkt *pkt, bool is_loopback)
{
again:
	switch (process_data(pkt, is_loopback)) {
	case NET_CONTINUE:
		if (IS_ENABLED(CONFIG_NET_L2_VIRTUAL)) {
			/* If we have a tunneling packet, feed it back
			 * to the stack in this case.
			 */
			goto again;
		} else {
			NET_DBG("Dropping pkt %p", pkt);
			net_pkt_unref(pkt);
		}
		break;
	case NET_OK:
		NET_DBG("Consumed pkt %p", pkt);
		break;
	case NET_DROP:
	default:
		NET_DBG("Dropping pkt %p", pkt);
		net_pkt_unref(pkt);
		break;
	}
}

/* Things to setup after we are able to RX and TX */
static void net_post_init(void)
{
#if defined(CONFIG_NET_LLDP)
	net_lldp_init();
#endif
#if defined(CONFIG_NET_GPTP)
	net_gptp_init();
#endif
}

static void init_rx_queues(void)
{
	/* Starting TX side. The ordering is important here and the TX
	 * can only be started when RX side is ready to receive packets.
	 */
	net_if_init();

	net_tc_rx_init();

	/* This will take the interface up and start everything. */
	net_if_post_init();

	/* Things to init after network interface is working */
	net_post_init();
}

/* If loopback driver is enabled, then direct packets to it so the address
 * check is not needed.
 */
#if defined(CONFIG_NET_IP) && defined(CONFIG_NET_IP_ADDR_CHECK) && !defined(CONFIG_NET_LOOPBACK)
/* Check if the IPv{4|6} addresses are proper. As this can be expensive,
 * make this optional.
 */
static inline int check_ip_addr(struct net_pkt *pkt)
{
	uint8_t family = net_pkt_family(pkt);

	if (IS_ENABLED(CONFIG_NET_IPV6) && family == AF_INET6) {
		if (net_ipv6_addr_cmp((struct in6_addr *)NET_IPV6_HDR(pkt)->dst,
				      net_ipv6_unspecified_address())) {
			NET_DBG("IPv6 dst address missing");
			return -EADDRNOTAVAIL;
		}

		/* If the destination address is our own, then route it
		 * back to us.
		 */
		if (net_ipv6_is_addr_loopback(
				(struct in6_addr *)NET_IPV6_HDR(pkt)->dst) ||
		    net_ipv6_is_my_addr(
				(struct in6_addr *)NET_IPV6_HDR(pkt)->dst)) {
			struct in6_addr addr;

			/* Swap the addresses so that in receiving side
			 * the packet is accepted.
			 */
			net_ipv6_addr_copy_raw((uint8_t *)&addr, NET_IPV6_HDR(pkt)->src);
			net_ipv6_addr_copy_raw(NET_IPV6_HDR(pkt)->src,
					       NET_IPV6_HDR(pkt)->dst);
			net_ipv6_addr_copy_raw(NET_IPV6_HDR(pkt)->dst, (uint8_t *)&addr);

			return 1;
		}

		/* If the destination address is interface local scope
		 * multicast address, then loop the data back to us.
		 * The FF01:: multicast addresses are only meant to be used
		 * in local host, so this is similar as how ::1 unicast
		 * addresses are handled. See RFC 3513 ch 2.7 for details.
		 */
		if (net_ipv6_is_addr_mcast_iface(
				(struct in6_addr *)NET_IPV6_HDR(pkt)->dst)) {
			NET_DBG("IPv6 interface scope mcast dst address");
			return 1;
		}

		/* The source check must be done after the destination check
		 * as having src ::1 is perfectly ok if dst is ::1 too.
		 */
		if (net_ipv6_is_addr_loopback(
				(struct in6_addr *)NET_IPV6_HDR(pkt)->src)) {
			NET_DBG("IPv6 loopback src address");
			return -EADDRNOTAVAIL;
		}
	} else if (IS_ENABLED(CONFIG_NET_IPV4) && family == AF_INET) {
		if (net_ipv4_addr_cmp((struct in_addr *)NET_IPV4_HDR(pkt)->dst,
				      net_ipv4_unspecified_address())) {
			NET_DBG("IPv4 dst address missing");
			return -EADDRNOTAVAIL;
		}

		/* If the destination address is our own, then route it
		 * back to us.
		 */
		if (net_ipv4_is_addr_loopback((struct in_addr *)NET_IPV4_HDR(pkt)->dst) ||
		    (net_ipv4_is_addr_bcast(net_pkt_iface(pkt),
				     (struct in_addr *)NET_IPV4_HDR(pkt)->dst) == false &&
		     net_ipv4_is_my_addr((struct in_addr *)NET_IPV4_HDR(pkt)->dst))) {
			struct in_addr addr;

			/* Swap the addresses so that in receiving side
			 * the packet is accepted.
			 */
			net_ipv4_addr_copy_raw((uint8_t *)&addr, NET_IPV4_HDR(pkt)->src);
			net_ipv4_addr_copy_raw(NET_IPV4_HDR(pkt)->src,
					       NET_IPV4_HDR(pkt)->dst);
			net_ipv4_addr_copy_raw(NET_IPV4_HDR(pkt)->dst, (uint8_t *)&addr);

			return 1;
		}

		/* The source check must be done after the destination check
		 * as having src 127.0.0.0/8 is perfectly ok if dst is in
		 * localhost subnet too.
		 */
		if (net_ipv4_is_addr_loopback((struct in_addr *)NET_IPV4_HDR(pkt)->src)) {
			NET_DBG("IPv4 loopback src address");
			return -EADDRNOTAVAIL;
		}
	}

	return 0;
}
#else
#define check_ip_addr(pkt) 0
#endif

/* Called when data needs to be sent to network */
int net_send_data(struct net_pkt *pkt)
{
	int status;

	if (!pkt || !pkt->frags) {
		return -ENODATA;
	}

	if (!net_pkt_iface(pkt)) {
		return -EINVAL;
	}

#if defined(CONFIG_NET_STATISTICS)
	switch (net_pkt_family(pkt)) {
	case AF_INET:
		net_stats_update_ipv4_sent(net_pkt_iface(pkt));
		break;
	case AF_INET6:
		net_stats_update_ipv6_sent(net_pkt_iface(pkt));
		break;
	}
#endif

	net_pkt_trim_buffer(pkt);
	net_pkt_cursor_init(pkt);

	status = check_ip_addr(pkt);
	if (status < 0) {
		return status;
	} else if (status > 0) {
		/* Packet is destined back to us so send it directly
		 * to RX processing.
		 */
		NET_DBG("Loopback pkt %p back to us", pkt);
		processing_data(pkt, true);
		return 0;
	}

	if (net_if_send_data(net_pkt_iface(pkt), pkt) == NET_DROP) {
		return -EIO;
	}

	return 0;
}

static void net_rx(struct net_if *iface, struct net_pkt *pkt)
{
	bool is_loopback = false;
	size_t pkt_len;

	pkt_len = net_pkt_get_len(pkt);

	NET_DBG("Received pkt %p len %zu", pkt, pkt_len);

	net_stats_update_bytes_recv(iface, pkt_len);

	if (IS_ENABLED(CONFIG_NET_LOOPBACK)) {
#ifdef CONFIG_NET_L2_DUMMY
		if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
			is_loopback = true;
		}
#endif
	}

	processing_data(pkt, is_loopback);

	net_print_statistics();
	net_pkt_print();
}

void net_process_rx_packet(struct net_pkt *pkt)
{
	net_pkt_set_rx_stats_tick(pkt, k_cycle_get_32());

	net_capture_pkt(net_pkt_iface(pkt), pkt);

	net_rx(net_pkt_iface(pkt), pkt);
}

static void net_queue_rx(struct net_if *iface, struct net_pkt *pkt)
{
	uint8_t prio = net_pkt_priority(pkt);
	uint8_t tc = net_rx_priority2tc(prio);

#if defined(CONFIG_NET_STATISTICS)
	net_stats_update_tc_recv_pkt(iface, tc);
	net_stats_update_tc_recv_bytes(iface, tc, net_pkt_get_len(pkt));
	net_stats_update_tc_recv_priority(iface, tc, prio);
#endif

#if NET_TC_RX_COUNT > 1
	NET_DBG("TC %d with prio %d pkt %p", tc, prio, pkt);
#endif

	if (NET_TC_RX_COUNT == 0) {
		net_process_rx_packet(pkt);
	} else {
		net_tc_submit_to_rx_queue(tc, pkt);
	}
}

/* Called by driver when a packet has been received */
int net_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	if (!pkt || !iface) {
		return -EINVAL;
	}

	if (net_pkt_is_empty(pkt)) {
		return -ENODATA;
	}

	if (!net_if_flag_is_set(iface, NET_IF_UP)) {
		return -ENETDOWN;
	}

	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_init(pkt);

	NET_DBG("prio %d iface %p pkt %p len %zu", net_pkt_priority(pkt),
		iface, pkt, net_pkt_get_len(pkt));

	if (IS_ENABLED(CONFIG_NET_ROUTING)) {
		net_pkt_set_orig_iface(pkt, iface);
	}

	net_pkt_set_iface(pkt, iface);

	if (!net_pkt_filter_recv_ok(pkt)) {
		/* silently drop the packet */
		net_pkt_unref(pkt);
	} else {
		net_queue_rx(iface, pkt);
	}

	return 0;
}

static inline void l3_init(void)
{
	net_icmpv4_init();
	net_icmpv6_init();
	net_ipv6_init();

	net_ipv4_autoconf_init();

	if (IS_ENABLED(CONFIG_NET_UDP) ||
	    IS_ENABLED(CONFIG_NET_TCP) ||
	    IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) ||
	    IS_ENABLED(CONFIG_NET_SOCKETS_CAN)) {
		net_conn_init();
	}

	net_tcp_init();

	net_route_init();

	NET_DBG("Network L3 init done");
}

static inline int services_init(void)
{
	int status;

	status = net_dhcpv4_init();
	if (status) {
		return status;
	}

	dns_init_resolver();
	websocket_init();

	net_coap_init();

	net_shell_init();

	return status;
}

static int net_init(const struct device *unused)
{
	net_hostname_init();

	NET_DBG("Priority %d", CONFIG_NET_INIT_PRIO);

	net_pkt_init();

	net_context_init();

	l3_init();

	net_mgmt_event_init();

	init_rx_queues();

	return services_init();
}

SYS_INIT(net_init, POST_KERNEL, CONFIG_NET_INIT_PRIO);
