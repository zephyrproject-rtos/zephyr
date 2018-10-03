/** @file
 * @brief IPv4 related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_IPV4)
#define SYS_LOG_DOMAIN "net/ipv4"
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_stats.h>
#include <net/net_context.h>
#include <net/tcp.h>
#include "net_private.h"
#include "connection.h"
#include "net_stats.h"
#include "icmpv4.h"
#include "udp_internal.h"
#include "tcp_internal.h"
#include "ipv4.h"

/* Timeout for various buffer allocations in this file. */
#define NET_BUF_TIMEOUT K_MSEC(50)

struct net_pkt *net_ipv4_create(struct net_pkt *pkt,
				const struct in_addr *src,
				const struct in_addr *dst,
				struct net_if *iface,
				u8_t next_header_proto)
{
	struct net_buf *header;

	header = net_pkt_get_frag(pkt, NET_BUF_TIMEOUT);
	if (!header) {
		return NULL;
	}

	net_pkt_frag_insert(pkt, header);

	NET_IPV4_HDR(pkt)->vhl = 0x45;
	NET_IPV4_HDR(pkt)->tos = 0x00;
	NET_IPV4_HDR(pkt)->proto = next_header_proto;

	/* User can tweak the default TTL if needed */
	NET_IPV4_HDR(pkt)->ttl = net_pkt_ipv4_ttl(pkt);
	if (NET_IPV4_HDR(pkt)->ttl == 0) {
		NET_IPV4_HDR(pkt)->ttl = net_if_ipv4_get_ttl(iface);
	}

	NET_IPV4_HDR(pkt)->offset[0] = NET_IPV4_HDR(pkt)->offset[1] = 0;
	NET_IPV4_HDR(pkt)->id[0] = NET_IPV4_HDR(pkt)->id[1] = 0;

	net_ipaddr_copy(&NET_IPV4_HDR(pkt)->dst, dst);
	net_ipaddr_copy(&NET_IPV4_HDR(pkt)->src, src);

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));
	net_pkt_set_family(pkt, AF_INET);

	net_buf_add(header, sizeof(struct net_ipv4_hdr));

	return pkt;
}

void net_ipv4_finalize(struct net_pkt *pkt, u8_t next_header_proto)
{
	/* Set the length of the IPv4 header */
	size_t total_len;

	net_pkt_compact(pkt);

	total_len = net_pkt_get_len(pkt);

	NET_IPV4_HDR(pkt)->len = htons(total_len);

	NET_IPV4_HDR(pkt)->chksum = 0;

	if (net_if_need_calc_tx_checksum(net_pkt_iface(pkt)) ||
	    next_header_proto == IPPROTO_ICMP) {
		NET_IPV4_HDR(pkt)->chksum = ~net_calc_chksum_ipv4(pkt);

		if (IS_ENABLED(CONFIG_NET_UDP) &&
		    next_header_proto == IPPROTO_UDP) {
			net_udp_set_chksum(pkt, pkt->frags);
		} else if (IS_ENABLED(CONFIG_NET_TCP) &&
			   next_header_proto == IPPROTO_TCP) {
			net_tcp_set_chksum(pkt, pkt->frags);
		} else if (next_header_proto == IPPROTO_ICMP) {
			net_icmpv4_set_chksum(pkt);
		}
	}
}

const struct in_addr *net_ipv4_unspecified_address(void)
{
	static const struct in_addr addr;

	return &addr;
}

const struct in_addr *net_ipv4_broadcast_address(void)
{
	static const struct in_addr addr = { { { 255, 255, 255, 255 } } };

	return &addr;
}

enum net_verdict net_ipv4_process_pkt(struct net_pkt *pkt)
{
	struct net_ipv4_hdr *hdr = NET_IPV4_HDR(pkt);
	int real_len = net_pkt_get_len(pkt);
	int pkt_len = ntohs(hdr->len);
	enum net_verdict verdict = NET_DROP;

	if (real_len != pkt_len) {
		NET_DBG("IPv4 packet size %d pkt len %d", pkt_len, real_len);
		goto drop;
	}

	NET_DBG("IPv4 packet received from %s to %s",
		net_sprint_ipv4_addr(&hdr->src),
		net_sprint_ipv4_addr(&hdr->dst));

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));
	net_pkt_set_ipv4_ttl(pkt, NET_IPV4_HDR(pkt)->ttl);

	if (!net_is_my_ipv4_addr(&hdr->dst) &&
	    !net_is_ipv4_addr_mcast(&hdr->dst)) {
		if (IS_ENABLED(CONFIG_NET_DHCPV4) &&
		    hdr->proto == IPPROTO_UDP &&
		    net_ipv4_addr_cmp(&hdr->dst,
				      net_ipv4_broadcast_address())) {

			verdict = net_conn_input(IPPROTO_UDP, pkt);
			if (verdict != NET_DROP) {
				return verdict;
			}
		}

		NET_DBG("IPv4 packet in pkt %p not for me", pkt);
		goto drop;
	}

	switch (hdr->proto) {
	case IPPROTO_ICMP:
		verdict = net_icmpv4_input(pkt);
		break;
	case IPPROTO_TCP:
	case IPPROTO_UDP:
		verdict = net_conn_input(hdr->proto, pkt);
		break;
	}

	if (verdict != NET_DROP) {
		return verdict;
	}

drop:
	net_stats_update_ipv4_drop(net_pkt_iface(pkt));
	return NET_DROP;
}
