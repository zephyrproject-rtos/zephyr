/** @file
 * @brief IPv4 related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_ipv4, CONFIG_NET_IPV4_LOG_LEVEL);

#include <errno.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_stats.h>
#include <net/net_context.h>
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
	NET_IPV4_HDR(pkt)->chksum = 0;

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

int net_ipv4_create_new(struct net_pkt *pkt,
			const struct in_addr *src,
			const struct in_addr *dst)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	struct net_ipv4_hdr *ipv4_hdr;

	ipv4_hdr = (struct net_ipv4_hdr *)net_pkt_get_data_new(pkt,
							       &ipv4_access);
	if (!ipv4_hdr) {
		return -ENOBUFS;
	}

	ipv4_hdr->vhl       = 0x45;
	ipv4_hdr->tos       = 0x00;
	ipv4_hdr->len       = 0;
	ipv4_hdr->id[0]     = 0;
	ipv4_hdr->id[1]     = 0;
	ipv4_hdr->offset[0] = 0;
	ipv4_hdr->offset[1] = 0;

	ipv4_hdr->ttl       = net_pkt_ipv4_ttl(pkt);
	if (ipv4_hdr->ttl == 0) {
		ipv4_hdr->ttl = net_if_ipv4_get_ttl(net_pkt_iface(pkt));
	}

	ipv4_hdr->proto     = 0;
	ipv4_hdr->chksum    = 0;

	net_ipaddr_copy(&ipv4_hdr->dst, dst);
	net_ipaddr_copy(&ipv4_hdr->src, src);

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	return net_pkt_set_data(pkt, &ipv4_access);
}

int net_ipv4_finalize(struct net_pkt *pkt, u8_t next_header_proto)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	struct net_ipv4_hdr *ipv4_hdr;

	net_pkt_set_overwrite(pkt, true);

	ipv4_hdr = (struct net_ipv4_hdr *)net_pkt_get_data_new(pkt,
							       &ipv4_access);
	if (!ipv4_hdr) {
		return -ENOBUFS;
	}

	ipv4_hdr->len   = htons(net_pkt_get_len(pkt));
	ipv4_hdr->proto = next_header_proto;

	if (net_if_need_calc_tx_checksum(net_pkt_iface(pkt)) ||
	    next_header_proto == IPPROTO_ICMP) {
		ipv4_hdr->chksum = net_calc_chksum_ipv4(pkt);

		net_pkt_set_data(pkt, &ipv4_access);

		if (IS_ENABLED(CONFIG_NET_UDP) &&
		    next_header_proto == IPPROTO_UDP) {
			return net_udp_finalize(pkt);
		} else if (IS_ENABLED(CONFIG_NET_TCP) &&
			   next_header_proto == IPPROTO_TCP) {
			return net_tcp_finalize(pkt);
		} else if (next_header_proto == IPPROTO_ICMP) {
			return net_icmpv4_finalize(pkt);
		}
	}

	return net_pkt_set_data(pkt, &ipv4_access);
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

enum net_verdict net_ipv4_input(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
	NET_PKT_DATA_ACCESS_DEFINE(tcp_access, struct net_tcp_hdr);
	int real_len = net_pkt_get_len(pkt);
	enum net_verdict verdict = NET_DROP;
	union net_proto_header proto_hdr;
	struct net_ipv4_hdr *hdr;
	union net_ip_header ip;
	int pkt_len;

	net_stats_update_ipv4_recv(net_pkt_iface(pkt));

	hdr = (struct net_ipv4_hdr *)net_pkt_get_data_new(pkt, &ipv4_access);
	if (!hdr) {
		NET_DBG("DROP: no buffer");
		goto drop;
	}

	pkt_len = ntohs(hdr->len);
	if (real_len < pkt_len) {
		NET_DBG("DROP: pkt len per hdr %d != pkt real len %d",
			pkt_len, real_len);
		goto drop;
	} else if (real_len > pkt_len) {
		net_pkt_update_length(pkt, pkt_len);
	}

	if (net_ipv4_is_addr_mcast(&hdr->src)) {
		NET_DBG("DROP: src addr is mcast");
		goto drop;
	}

	if (net_ipv4_is_addr_bcast(net_pkt_iface(pkt), &hdr->src)) {
		NET_DBG("DROP: src addr is bcast");
		goto drop;
	}

	if (net_if_need_calc_rx_checksum(net_pkt_iface(pkt)) &&
	    net_calc_chksum_ipv4(pkt) != 0) {
		NET_DBG("DROP: invalid chksum");
		goto drop;
	}

	if (!net_ipv4_is_my_addr(&hdr->dst) &&
	    !net_ipv4_is_addr_mcast(&hdr->dst) &&
	    ((hdr->proto == IPPROTO_UDP &&
	      net_ipv4_addr_cmp(&hdr->dst, net_ipv4_broadcast_address()) &&
	      !IS_ENABLED(CONFIG_NET_DHCPV4)) ||
	     (hdr->proto == IPPROTO_TCP &&
	      net_ipv4_is_addr_bcast(net_pkt_iface(pkt), &hdr->dst)))) {
		NET_DBG("DROP: not for me");
		goto drop;
	}

	NET_DBG("IPv4 packet received from %s to %s",
		log_strdup(net_sprint_ipv4_addr(&hdr->src)),
		log_strdup(net_sprint_ipv4_addr(&hdr->dst)));

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));
	net_pkt_set_ipv4_ttl(pkt, hdr->ttl);

	net_pkt_set_family(pkt, PF_INET);

	net_pkt_acknowledge_data(pkt, &ipv4_access);

	switch (hdr->proto) {
	case IPPROTO_ICMP:
		verdict = net_icmpv4_input(pkt, hdr);
		break;
	case IPPROTO_TCP:
		proto_hdr.tcp = net_tcp_input(pkt, &tcp_access);
		if (proto_hdr.tcp) {
			verdict = NET_OK;
		}
		break;
	case IPPROTO_UDP:
		proto_hdr.udp = net_udp_input(pkt, &udp_access);
		if (proto_hdr.udp) {
			verdict = NET_OK;
		}
		break;
	}

	if (verdict == NET_DROP) {
		goto drop;
	} else if (hdr->proto == IPPROTO_ICMP) {
		return verdict;
	}

	ip.ipv4 = hdr;

	verdict = net_conn_input(pkt, &ip, hdr->proto, &proto_hdr);
	if (verdict != NET_DROP) {
		return verdict;
	}
drop:
	net_stats_update_ipv4_drop(net_pkt_iface(pkt));
	return NET_DROP;
}
