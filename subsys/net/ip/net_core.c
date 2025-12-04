/** @file
 * @brief Network initialization
 *
 * Initialize the network IP stack. Create one thread for reading data
 * from IP stack and passing that data to applications (Rx thread).
 */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2025 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_core, CONFIG_NET_CORE_LOG_LEVEL);

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <string.h>
#include <errno.h>

#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/ipv4_autoconf.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/gptp.h>
#include <zephyr/net/websocket.h>
#include <zephyr/net/ethernet.h>
#if defined(CONFIG_NET_DSA) && !defined(CONFIG_NET_DSA_DEPRECATED)
#include <zephyr/net/dsa_core.h>
#endif
#include <zephyr/net/capture.h>

#if defined(CONFIG_NET_LLDP)
#include <zephyr/net/lldp.h>
#endif

#include "net_private.h"
#include "shell/net_shell.h"

#include "pmtu.h"

#include "icmpv6.h"
#include "ipv6.h"

#include "icmpv4.h"
#include "ipv4.h"

#include "dhcpv4/dhcpv4_internal.h"
#include "dhcpv6/dhcpv6_internal.h"

#include "route.h"

#include "packet_socket.h"
#include "canbus_socket.h"

#include "connection.h"
#include "udp_internal.h"
#include "tcp_internal.h"

#include "net_stats.h"

#if defined(CONFIG_NET_NATIVE)
static inline enum net_verdict process_data(struct net_pkt *pkt)
{
	int ret;

	net_packet_socket_input(pkt, ETH_P_ALL, NET_SOCK_RAW);

	/* If there is no data, then drop the packet. */
	if (!pkt->frags) {
		NET_DBG("Corrupted packet (frags %p)", pkt->frags);
		net_stats_update_processing_error(net_pkt_iface(pkt));

		return NET_DROP;
	}

	if (!net_pkt_is_l2_processed(pkt)) {
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
		net_packet_socket_input(pkt, net_pkt_ll_proto_type(pkt), NET_SOCK_DGRAM);
	}

	uint8_t family = net_pkt_family(pkt);

	if (IS_ENABLED(CONFIG_NET_IP) && (family == NET_AF_INET || family == NET_AF_INET6 ||
					  family == NET_AF_UNSPEC || family == NET_AF_PACKET)) {
		/* IP version and header length. */
		uint8_t vtc_vhl = NET_IPV6_HDR(pkt)->vtc & 0xf0;

		if (IS_ENABLED(CONFIG_NET_IPV6) && vtc_vhl == 0x60) {
			return net_ipv6_input(pkt);
		} else if (IS_ENABLED(CONFIG_NET_IPV4) && vtc_vhl == 0x40) {
			return net_ipv4_input(pkt);
		}

		NET_DBG("Unknown IP family packet (0x%x)", NET_IPV6_HDR(pkt)->vtc & 0xf0);
		net_stats_update_ip_errors_protoerr(net_pkt_iface(pkt));
		net_stats_update_ip_errors_vhlerr(net_pkt_iface(pkt));
		return NET_DROP;
	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_CAN) && family == NET_AF_CAN) {
		return net_canbus_socket_input(pkt);
	}

	NET_DBG("Unknown protocol family packet (0x%x)", family);
	return NET_DROP;
}

static void processing_data(struct net_pkt *pkt)
{
again:
	switch (process_data(pkt)) {
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

static inline void copy_ll_addr(struct net_pkt *pkt)
{
	memcpy(net_pkt_lladdr_src(pkt), net_pkt_lladdr_if(pkt),
	       sizeof(struct net_linkaddr));
	memcpy(net_pkt_lladdr_dst(pkt), net_pkt_lladdr_if(pkt),
	       sizeof(struct net_linkaddr));
}

/* Check if the IPv{4|6} addresses are proper. As this can be expensive,
 * make this optional. We still check the IPv4 TTL and IPv6 hop limit
 * if the corresponding protocol family is enabled.
 */
static inline int check_ip(struct net_pkt *pkt)
{
	uint8_t family;
	int ret;

	if (!IS_ENABLED(CONFIG_NET_IP)) {
		return 0;
	}

	family = net_pkt_family(pkt);
	ret = 0;

	if (IS_ENABLED(CONFIG_NET_IPV6) && family == NET_AF_INET6 &&
	    net_pkt_ll_proto_type(pkt) == NET_ETH_PTYPE_IPV6) {
		/* Drop IPv6 packet if hop limit is 0 */
		if (NET_IPV6_HDR(pkt)->hop_limit == 0) {
			NET_DBG("DROP: IPv6 hop limit");
			ret = -ENOMSG; /* silently drop the pkt, not an error */
			goto drop;
		}

		if (!IS_ENABLED(CONFIG_NET_IP_ADDR_CHECK)) {
			return 0;
		}

#if defined(CONFIG_NET_LOOPBACK)
		/* If loopback driver is enabled, then send packets to it
		 * as the address check is not needed.
		 */
		if (net_if_l2(net_pkt_iface(pkt)) == &NET_L2_GET_NAME(DUMMY)) {
			return 0;
		}
#endif
		if (net_ipv6_addr_cmp_raw(NET_IPV6_HDR(pkt)->dst,
					  (const uint8_t *)net_ipv6_unspecified_address())) {
			NET_DBG("DROP: IPv6 dst address missing");
			ret = -EADDRNOTAVAIL;
			goto drop;
		}

		/* If the destination address is our own, then route it
		 * back to us (if it is not already forwarded).
		 */
		if ((net_ipv6_is_addr_loopback_raw(NET_IPV6_HDR(pkt)->dst) ||
		    net_ipv6_is_my_addr_raw(NET_IPV6_HDR(pkt)->dst)) &&
		    !net_pkt_forwarding(pkt)) {
			struct net_in6_addr addr;

			/* Swap the addresses so that in receiving side
			 * the packet is accepted.
			 */
			net_ipv6_addr_copy_raw((uint8_t *)&addr, NET_IPV6_HDR(pkt)->src);
			net_ipv6_addr_copy_raw(NET_IPV6_HDR(pkt)->src,
					       NET_IPV6_HDR(pkt)->dst);
			net_ipv6_addr_copy_raw(NET_IPV6_HDR(pkt)->dst, (uint8_t *)&addr);

			net_pkt_set_ll_proto_type(pkt, ETH_P_IPV6);
			copy_ll_addr(pkt);

			return 1;
		}

		/* If the destination address is interface local scope
		 * multicast address, then loop the data back to us.
		 * The FF01:: multicast addresses are only meant to be used
		 * in local host, so this is similar as how ::1 unicast
		 * addresses are handled. See RFC 3513 ch 2.7 for details.
		 */
		if (net_ipv6_is_addr_mcast_iface_raw(NET_IPV6_HDR(pkt)->dst)) {
			NET_DBG("IPv6 interface scope mcast dst address");
			return 1;
		}

		/* The source check must be done after the destination check
		 * as having src ::1 is perfectly ok if dst is ::1 too.
		 */
		if (net_ipv6_is_addr_loopback_raw(NET_IPV6_HDR(pkt)->src)) {
			NET_DBG("DROP: IPv6 loopback src address");
			ret = -EADDRNOTAVAIL;
			goto drop;
		}

	} else if (IS_ENABLED(CONFIG_NET_IPV4) && family == NET_AF_INET &&
		   net_pkt_ll_proto_type(pkt) == NET_ETH_PTYPE_IP) {
		/* Drop IPv4 packet if ttl is 0 */
		if (NET_IPV4_HDR(pkt)->ttl == 0) {
			NET_DBG("DROP: IPv4 ttl");
			ret = -ENOMSG; /* silently drop the pkt, not an error */
			goto drop;
		}

		if (!IS_ENABLED(CONFIG_NET_IP_ADDR_CHECK)) {
			return 0;
		}

#if defined(CONFIG_NET_LOOPBACK)
		/* If loopback driver is enabled, then send packets to it
		 * as the address check is not needed.
		 */
		if (net_if_l2(net_pkt_iface(pkt)) == &NET_L2_GET_NAME(DUMMY)) {
			return 0;
		}
#endif
		if (net_ipv4_addr_cmp_raw(NET_IPV4_HDR(pkt)->dst,
					  net_ipv4_unspecified_address()->s4_addr)) {
			NET_DBG("DROP: IPv4 dst address missing");
			ret = -EADDRNOTAVAIL;
			goto drop;
		}

		/* If the destination address is our own, then route it
		 * back to us.
		 */
		if (net_ipv4_is_addr_loopback_raw(NET_IPV4_HDR(pkt)->dst) ||
		    (net_ipv4_is_addr_bcast_raw(net_pkt_iface(pkt),
						NET_IPV4_HDR(pkt)->dst) == false &&
		     net_ipv4_is_my_addr_raw(NET_IPV4_HDR(pkt)->dst))) {
			struct net_in_addr addr;

			/* Swap the addresses so that in receiving side
			 * the packet is accepted.
			 */
			net_ipv4_addr_copy_raw((uint8_t *)&addr, NET_IPV4_HDR(pkt)->src);
			net_ipv4_addr_copy_raw(NET_IPV4_HDR(pkt)->src,
					       NET_IPV4_HDR(pkt)->dst);
			net_ipv4_addr_copy_raw(NET_IPV4_HDR(pkt)->dst, (uint8_t *)&addr);

			net_pkt_set_ll_proto_type(pkt, ETH_P_IP);
			copy_ll_addr(pkt);

			return 1;
		}

		/* The source check must be done after the destination check
		 * as having src 127.0.0.0/8 is perfectly ok if dst is in
		 * localhost subnet too.
		 */
		if (net_ipv4_is_addr_loopback_raw(NET_IPV4_HDR(pkt)->src)) {
			NET_DBG("DROP: IPv4 loopback src address");
			ret = -EADDRNOTAVAIL;
			goto drop;
		}
	}

	return ret;

drop:
	if (IS_ENABLED(CONFIG_NET_STATISTICS)) {
		if (family == NET_AF_INET6) {
			net_stats_update_ipv6_drop(net_pkt_iface(pkt));
		} else {
			net_stats_update_ipv4_drop(net_pkt_iface(pkt));
		}
	}

	return ret;
}

#if defined(CONFIG_NET_IPV4) || defined(CONFIG_NET_IPV6)
static inline bool process_multicast(struct net_pkt *pkt)
{
	struct net_context *ctx = net_pkt_context(pkt);
	net_sa_family_t family = net_pkt_family(pkt);

	if (ctx == NULL) {
		return false;
	}

#if defined(CONFIG_NET_IPV4)
	if (family == NET_AF_INET) {
		const struct net_in_addr *dst = (const struct net_in_addr *)&NET_IPV4_HDR(pkt)->dst;

		return net_ipv4_is_addr_mcast(dst) && net_context_get_ipv4_mcast_loop(ctx);
	}
#endif
#if defined(CONFIG_NET_IPV6)
	if (family == NET_AF_INET6) {
		return net_ipv6_is_addr_mcast_raw(NET_IPV6_HDR(pkt)->dst) &&
		       net_context_get_ipv6_mcast_loop(ctx);
	}
#endif
	return false;
}
#endif

int net_try_send_data(struct net_pkt *pkt, k_timeout_t timeout)
{
	struct net_if *iface;
	int family;
	int status;
	int ret;

	SYS_PORT_TRACING_FUNC_ENTER(net, send_data, pkt);

	if (!pkt || !pkt->frags) {
		ret = -ENODATA;
		goto err;
	}

	if (!net_pkt_iface(pkt)) {
		ret = -EINVAL;
		goto err;
	}

	if (!net_if_is_up(net_pkt_iface(pkt))) {
		ret = -ENETDOWN;
		goto err;
	}

	net_pkt_trim_buffer(pkt);
	net_pkt_cursor_init(pkt);

	status = check_ip(pkt);
	if (status < 0) {
		/* Special handling for ENOMSG which is returned if packet
		 * TTL is 0 or hop limit is 0. This is not an error as it is
		 * perfectly valid case to set the limit to 0. In this case
		 * we just silently drop the packet by returning 0.
		 */
		if (status == -ENOMSG) {
			net_pkt_unref(pkt);
			ret = 0;
			goto err;
		}

		return status;
	} else if (status > 0) {
		/* Packet is destined back to us so send it directly
		 * to RX processing.
		 */
		NET_DBG("Loopback pkt %p back to us", pkt);
		net_pkt_set_loopback(pkt, true);
		net_pkt_set_l2_processed(pkt, true);
		processing_data(pkt);
		ret = 0;
		goto err;
	}

#if defined(CONFIG_NET_IPV4) || defined(CONFIG_NET_IPV6)
	if (process_multicast(pkt)) {
		struct net_pkt *clone = net_pkt_clone(pkt, K_NO_WAIT);

		if (clone != NULL) {
			net_pkt_set_iface(clone, net_pkt_iface(pkt));
			if (net_recv_data(net_pkt_iface(clone), clone) < 0) {
				if (IS_ENABLED(CONFIG_NET_STATISTICS)) {
					switch (net_pkt_family(pkt)) {
#if defined(CONFIG_NET_IPV4)
					case NET_AF_INET:
						net_stats_update_ipv4_sent(net_pkt_iface(pkt));
						break;
#endif
#if defined(CONFIG_NET_IPV6)
					case NET_AF_INET6:
						net_stats_update_ipv6_sent(net_pkt_iface(pkt));
						break;
#endif
					}
				}
				net_pkt_unref(clone);
			}
		} else {
			NET_DBG("Failed to clone multicast packet");
		}
	}
#endif

	/* The pkt might contain garbage already after the call to
	 * net_if_try_send_data(), so do not use pkt after that call.
	 * Remember the iface and family for statistics update.
	 */
	if (IS_ENABLED(CONFIG_NET_STATISTICS)) {
		iface = net_pkt_iface(pkt);
		family = net_pkt_family(pkt);
	}

	if (net_if_try_send_data(net_pkt_iface(pkt), pkt, timeout) == NET_DROP) {
		ret = -EIO;
		goto err;
	}

	if (IS_ENABLED(CONFIG_NET_STATISTICS)) {
		switch (family) {
		case NET_AF_INET:
			net_stats_update_ipv4_sent(iface);
			break;
		case NET_AF_INET6:
			net_stats_update_ipv6_sent(iface);
			break;
		}
	}

	ret = 0;

err:
	SYS_PORT_TRACING_FUNC_EXIT(net, send_data, pkt, ret);

	return ret;
}

static void net_rx(struct net_if *iface, struct net_pkt *pkt)
{
	size_t pkt_len;

	pkt_len = net_pkt_get_len(pkt);

	NET_DBG("Received pkt %p len %zu", pkt, pkt_len);

	net_stats_update_bytes_recv(iface, pkt_len);
	conn_mgr_if_used(iface);

	if (IS_ENABLED(CONFIG_NET_LOOPBACK)) {
#ifdef CONFIG_NET_L2_DUMMY
		if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
			net_pkt_set_loopback(pkt, true);
			net_pkt_set_l2_processed(pkt, true);
		}
#endif
	}

	processing_data(pkt);

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
	size_t len = net_pkt_get_len(pkt);
	uint8_t prio = net_pkt_priority(pkt);
	uint8_t tc = net_rx_priority2tc(prio);

#if NET_TC_RX_COUNT > 1
	NET_DBG("TC %d with prio %d pkt %p", tc, prio, pkt);
#endif
	if (net_tc_rx_is_immediate(tc, prio)) {
		net_process_rx_packet(pkt);
	} else {
		if (net_tc_submit_to_rx_queue(tc, pkt) != NET_OK) {
			goto drop;
		}
	}

	net_stats_update_tc_recv_pkt(iface, tc);
	net_stats_update_tc_recv_bytes(iface, tc, len);
	net_stats_update_tc_recv_priority(iface, tc, prio);
	return;

drop:
	net_pkt_unref(pkt);
	net_stats_update_tc_recv_dropped(iface, tc);
	return;
}

/* Called by driver when a packet has been received */
int net_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	int ret;
#if defined(CONFIG_NET_DSA) && !defined(CONFIG_NET_DSA_DEPRECATED)
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);

	/* DSA driver handles first to untag and to redirect to user interface. */
	if (eth_ctx != NULL && (eth_ctx->dsa_port == DSA_CONDUIT_PORT)) {
		iface = dsa_recv(iface, pkt);
	}
#endif

	SYS_PORT_TRACING_FUNC_ENTER(net, recv_data, iface, pkt);

	if (!pkt || !iface) {
		ret = -EINVAL;
		goto err;
	}

	if (net_pkt_is_empty(pkt)) {
		ret = -ENODATA;
		goto err;
	}

	if (!net_if_flag_is_set(iface, NET_IF_UP)) {
		ret = -ENETDOWN;
		goto err;
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
		/* Silently drop the packet, but update the statistics in order
		 * to be able to monitor filter activity.
		 */
		net_stats_update_filter_rx_drop(net_pkt_iface(pkt));
		net_pkt_unref(pkt);
	} else {
		net_queue_rx(iface, pkt);
	}

	ret = 0;

err:
	SYS_PORT_TRACING_FUNC_EXIT(net, recv_data, iface, pkt, ret);

	return ret;
}

static inline void l3_init(void)
{
	net_pmtu_init();
	net_icmpv4_init();
	net_icmpv6_init();
	net_ipv4_init();
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
#else /* CONFIG_NET_NATIVE */
#define l3_init(...)
#define net_post_init(...)
int net_try_send_data(struct net_pkt *pkt, k_timeout_t timeout)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(timeout);

	return -ENOTSUP;
}
int net_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(pkt);

	return -ENOTSUP;
}
#endif /* CONFIG_NET_NATIVE */

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

static inline int services_init(void)
{
	int status;

	socket_service_init();

	status = net_dhcpv4_init();
	if (status) {
		return status;
	}

	status = net_dhcpv6_init();
	if (status != 0) {
		return status;
	}

	net_dhcpv4_server_init();

	dns_dispatcher_init();
	dns_init_resolver();
	mdns_init_responder();

	websocket_init();

	net_coap_init();

	net_shell_init();

	return status;
}

static int net_init(void)
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
