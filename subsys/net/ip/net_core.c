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

#if defined(CONFIG_NET_DEBUG_CORE)
#define SYS_LOG_DOMAIN "net/core"
#define NET_LOG_ENABLED 1
#endif

#include <init.h>
#include <kernel.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <string.h>
#include <errno.h>

#include <net/net_if.h>
#include <net/net_mgmt.h>
#include <net/arp.h>
#include <net/net_pkt.h>
#include <net/net_core.h>
#include <net/dns_resolve.h>
#include <net/gptp.h>

#include "net_private.h"
#include "net_shell.h"

#include "icmpv6.h"
#include "ipv6.h"

#include "icmpv4.h"

#if defined(CONFIG_NET_DHCPV4)
#include "dhcpv4.h"
#endif

#include "route.h"
#include "rpl.h"

#include "connection.h"
#include "udp_internal.h"
#include "tcp.h"

#include "net_stats.h"

/* Stack for the rx thread.
 */
#if !defined(CONFIG_NET_RX_STACK_SIZE)
#define CONFIG_NET_RX_STACK_SIZE 1024
#endif

NET_STACK_ARRAY_DEFINE(RX, rx_stack,
		       CONFIG_NET_RX_STACK_SIZE,
		       CONFIG_NET_RX_STACK_SIZE + CONFIG_NET_RX_STACK_RPL,
		       NET_TC_RX_COUNT);

static struct net_traffic_class classes[NET_TC_RX_COUNT];

static inline enum net_verdict process_data(struct net_pkt *pkt,
					    bool is_loopback)
{
	int ret;
	bool locally_routed = false;

#if defined(CONFIG_NET_IPV6_FRAGMENT)
	/* If the packet is routed back to us when we have reassembled
	 * an IPv6 packet, then do not pass it to L2 as the packet does
	 * not have link layer headers in it.
	 */
	if (net_pkt_ipv6_fragment_start(pkt)) {
		locally_routed = true;
	}
#endif

	/* If there is no data, then drop the packet. */
	if (!pkt->frags) {
		NET_DBG("Corrupted packet (frags %p)", pkt->frags);
		net_stats_update_processing_error();

		return NET_DROP;
	}

	if (!is_loopback && !locally_routed) {
		ret = net_if_recv_data(net_pkt_iface(pkt), pkt);
		if (ret != NET_CONTINUE) {
			if (ret == NET_DROP) {
				NET_DBG("Packet %p discarded by L2", pkt);
				net_stats_update_processing_error();
			}

			return ret;
		}
	}

	/* IP version and header length. */
	switch (NET_IPV6_HDR(pkt)->vtc & 0xf0) {
#if defined(CONFIG_NET_IPV6)
	case 0x60:
		net_stats_update_ipv6_recv();
		net_pkt_set_family(pkt, PF_INET6);
		return net_ipv6_process_pkt(pkt);
#endif
#if defined(CONFIG_NET_IPV4)
	case 0x40:
		net_stats_update_ipv4_recv();
		net_pkt_set_family(pkt, PF_INET);
		return net_ipv4_process_pkt(pkt);
#endif
	}

	NET_DBG("Unknown IP family packet (0x%x)",
		NET_IPV6_HDR(pkt)->vtc & 0xf0);
	net_stats_update_ip_errors_protoerr();
	net_stats_update_ip_errors_vhlerr();

	return NET_DROP;
}

static void processing_data(struct net_pkt *pkt, bool is_loopback)
{
	switch (process_data(pkt, is_loopback)) {
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
#if defined(CONFIG_NET_GPTP)
	net_gptp_init();
#endif
}

/* Convert traffic class to thread priority */
static int tc2thread(int tc)
{
	/* Initial implementation just maps the traffic class to certain queue.
	 * If there are less queues than classes, then map them into
	 * some specific queue. In order to make this work same way as before,
	 * the thread priority 7 is used to map the default traffic class so
	 * this system works same way as before when RX thread default priority
	 * was 7.
	 *
	 * Lower value in this table means higher thread priority. The
	 * value is used as a parameter to K_PRIO_COOP() which converts it
	 * to actual thread priority.
	 *
	 * Higher traffic class value means higher priority queue. This means
	 * that thread_priorities[7] value should contain the highest priority
	 * for the RX queue handling thread.
	 */
	static const int thread_priorities[] = {
#if NET_TC_RX_COUNT == 1
		7
#endif
#if NET_TC_RX_COUNT == 2
		8, 7
#endif
#if NET_TC_RX_COUNT == 3
		8, 7, 6
#endif
#if NET_TC_RX_COUNT == 4
		8, 7, 6, 5
#endif
#if NET_TC_RX_COUNT == 5
		8, 7, 6, 5, 4
#endif
#if NET_TC_RX_COUNT == 6
		8, 7, 6, 5, 4, 3
#endif
#if NET_TC_RX_COUNT == 7
		8, 7, 6, 5, 4, 3, 2
#endif
#if NET_TC_RX_COUNT == 8
		8, 7, 6, 5, 4, 3, 2, 1
#endif
	};

	BUILD_ASSERT_MSG(NET_TC_RX_COUNT <= CONFIG_NUM_COOP_PRIORITIES,
			 "Too many traffic classes");

	NET_ASSERT(tc < sizeof(thread_priorities));

	return thread_priorities[tc];
}

#if defined(CONFIG_NET_SHELL)
#define RX_STACK(idx) NET_STACK_GET_NAME(RX, rx_stack, 0)[idx].stack
#else
#define RX_STACK(idx) NET_STACK_GET_NAME(RX, rx_stack, 0)[idx]
#endif

#if defined(CONFIG_NET_STATISTICS)
/* Fixup the traffic class statistics so that "net stats" shell command will
 * print output correctly.
 */
static void tc_stats_priority_setup(void)
{
	int i;

	for (i = 0; i < 8; i++) {
		net_stats_update_tc_recv_priority(net_rx_priority2tc(i), i);
	}
}
#endif

static void init_rx_queues(void)
{
	int i;

	BUILD_ASSERT(NET_TC_RX_COUNT > 0);

#if defined(CONFIG_NET_STATISTICS)
	tc_stats_priority_setup();
#endif

	for (i = 0; i < NET_TC_RX_COUNT; i++) {
		u8_t thread_priority;

		thread_priority = tc2thread(i);
		classes[i].tc = thread_priority;

#if defined(CONFIG_NET_SHELL)
		/* Fix the thread start address so that "net stacks"
		 * command will print correct stack information.
		 */
		NET_STACK_GET_NAME(RX, rx_stack, 0)[i].stack = rx_stack[i];
		NET_STACK_GET_NAME(RX, rx_stack, 0)[i].prio = thread_priority;
		NET_STACK_GET_NAME(RX, rx_stack, 0)[i].idx = i;
#endif

		NET_DBG("[%d] Starting RX queue %p stack %p size %zd "
			"prio %d (%d)", i,
			&classes[i].work_q.queue, RX_STACK(i),
			K_THREAD_STACK_SIZEOF(rx_stack[i]),
			thread_priority, K_PRIO_COOP(thread_priority));

		k_work_q_start(&classes[i].work_q,
			       rx_stack[i],
			       K_THREAD_STACK_SIZEOF(rx_stack[i]),
			       K_PRIO_COOP(thread_priority));
	}

	/* Starting TX side. The ordering is important here and the TX
	 * can only be started when RX side is ready to receive packets.
	 * We synchronize the startup of the device so that both RX and TX
	 * are only started fully when both are ready to receive or send
	 * data.
	 */
	net_if_init();

	/* This will take the interface up and start everything. */
	net_if_post_init();

	/* Things to init after network interface is working */
	net_post_init();
}

/* If loopback driver is enabled, then direct packets to it so the address
 * check is not needed.
 */
#if defined(CONFIG_NET_IP_ADDR_CHECK) && !defined(CONFIG_NET_LOOPBACK)
/* Check if the IPv{4|6} addresses are proper. As this can be expensive,
 * make this optional.
 */
static inline int check_ip_addr(struct net_pkt *pkt)
{
#if defined(CONFIG_NET_IPV6)
	if (net_pkt_family(pkt) == AF_INET6) {
		if (net_ipv6_addr_cmp(&NET_IPV6_HDR(pkt)->dst,
				      net_ipv6_unspecified_address())) {
			NET_DBG("IPv6 dst address missing");
			return -EADDRNOTAVAIL;
		}

		/* If the destination address is our own, then route it
		 * back to us.
		 */
		if (net_is_ipv6_addr_loopback(&NET_IPV6_HDR(pkt)->dst) ||
		    net_is_my_ipv6_addr(&NET_IPV6_HDR(pkt)->dst)) {
			struct in6_addr addr;

			/* Swap the addresses so that in receiving side
			 * the packet is accepted.
			 */
			net_ipaddr_copy(&addr, &NET_IPV6_HDR(pkt)->src);
			net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src,
					&NET_IPV6_HDR(pkt)->dst);
			net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst, &addr);

			return 1;
		}

		/* The source check must be done after the destination check
		 * as having src ::1 is perfectly ok if dst is ::1 too.
		 */
		if (net_is_ipv6_addr_loopback(&NET_IPV6_HDR(pkt)->src)) {
			NET_DBG("IPv6 loopback src address");
			return -EADDRNOTAVAIL;
		}
	} else
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_pkt_family(pkt) == AF_INET) {
		if (net_ipv4_addr_cmp(&NET_IPV4_HDR(pkt)->dst,
				      net_ipv4_unspecified_address())) {
			NET_DBG("IPv4 dst address missing");
			return -EADDRNOTAVAIL;
		}

		/* If the destination address is our own, then route it
		 * back to us.
		 */
		if (net_is_ipv4_addr_loopback(&NET_IPV4_HDR(pkt)->dst) ||
		    net_is_my_ipv4_addr(&NET_IPV4_HDR(pkt)->dst)) {
			struct in_addr addr;

			/* Swap the addresses so that in receiving side
			 * the packet is accepted.
			 */
			net_ipaddr_copy(&addr, &NET_IPV4_HDR(pkt)->src);
			net_ipaddr_copy(&NET_IPV4_HDR(pkt)->src,
					&NET_IPV4_HDR(pkt)->dst);
			net_ipaddr_copy(&NET_IPV4_HDR(pkt)->dst, &addr);

			return 1;
		}

		/* The source check must be done after the destination check
		 * as having src 127.0.0.0/8 is perfectly ok if dst is in
		 * localhost subnet too.
		 */
		if (net_is_ipv4_addr_loopback(&NET_IPV4_HDR(pkt)->src)) {
			NET_DBG("IPv4 loopback src address");
			return -EADDRNOTAVAIL;
		}
	} else
#endif /* CONFIG_NET_IPV4 */

	{
		;
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
		net_stats_update_ipv4_sent();
		break;
	case AF_INET6:
		net_stats_update_ipv6_sent();
		break;
	}
#endif

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
#if defined(CONFIG_NET_STATISTICS) || defined(CONFIG_NET_DEBUG_CORE)
	size_t pkt_len;
#if defined(CONFIG_NET_STATISTICS)
	pkt_len = pkt->total_pkt_len;
#else
	pkt_len = net_pkt_get_len(pkt);
#endif
#endif

	NET_DBG("Received pkt %p len %zu", pkt, pkt_len);

	net_stats_update_bytes_recv(pkt_len);

	processing_data(pkt, false);

	net_print_statistics();
	net_pkt_print();
}

static void process_rx_packet(struct k_work *work)
{
	struct net_pkt *pkt;

	pkt = CONTAINER_OF(work, struct net_pkt, work);

	net_rx(net_pkt_iface(pkt), pkt);
}

static void net_queue_rx(struct net_if *iface, struct net_pkt *pkt)
{
	u8_t prio = net_pkt_priority(pkt);
	u8_t tc = net_rx_priority2tc(prio);

	k_work_init(net_pkt_work(pkt), process_rx_packet);

#if defined(CONFIG_NET_STATISTICS)
	pkt->total_pkt_len = net_pkt_get_len(pkt);

	net_stats_update_tc_recv_pkt(tc);
	net_stats_update_tc_recv_bytes(tc, pkt->total_pkt_len);
	net_stats_update_tc_recv_priority(tc, prio);
#endif

#if NET_TC_RX_COUNT > 1
	NET_DBG("TC %d with prio %d pkt %p", tc, prio, pkt);
#endif

	k_work_submit_to_queue(&classes[tc].work_q, net_pkt_work(pkt));
}

/* Called by driver when an IP packet has been received */
int net_recv_data(struct net_if *iface, struct net_pkt *pkt)
{
	NET_ASSERT(pkt && pkt->frags);
	NET_ASSERT(iface);

	if (!pkt->frags) {
		return -ENODATA;
	}

	if (!atomic_test_bit(iface->if_dev->flags, NET_IF_UP)) {
		return -ENETDOWN;
	}

	NET_DBG("prio %d iface %p pkt %p len %zu", net_pkt_priority(pkt),
		iface, pkt, net_pkt_get_len(pkt));

	if (IS_ENABLED(CONFIG_NET_ROUTING)) {
		net_pkt_set_orig_iface(pkt, iface);
	}

	net_pkt_set_iface(pkt, iface);

	net_queue_rx(iface, pkt);

	return 0;
}

static inline void l3_init(void)
{
	net_icmpv4_init();
	net_icmpv6_init();
	net_ipv6_init();

#if defined(CONFIG_NET_UDP) || defined(CONFIG_NET_TCP)
	net_conn_init();
#endif
	net_udp_init();
	net_tcp_init();

	net_route_init();

	dns_init_resolver();

	NET_DBG("Network L3 init done");
}

static inline void l2_init(void)
{
	net_arp_init();

	NET_DBG("Network L2 init done");
}

static int net_init(struct device *unused)
{
	int status = 0;

	net_hostname_init();

	NET_DBG("Priority %d", CONFIG_NET_INIT_PRIO);

	net_pkt_init();

	net_context_init();

	l2_init();
	l3_init();

	net_mgmt_event_init();

	init_rx_queues();

#if CONFIG_NET_DHCPV4
	status = dhcpv4_init();
	if (status) {
		return status;
	}
#endif

	return status;
}

SYS_INIT(net_init, POST_KERNEL, CONFIG_NET_INIT_PRIO);
