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

int net_ipv4_create(struct net_pkt *pkt,
		    const struct in_addr *src,
		    const struct in_addr *dst)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	struct net_ipv4_hdr *ipv4_hdr;

	ipv4_hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &ipv4_access);
	if (!ipv4_hdr) {
		return -ENOBUFS;
	}

	ipv4_hdr->vhl       = 0x45;
	ipv4_hdr->tos       = 0x00;
	ipv4_hdr->len       = 0U;
	ipv4_hdr->id[0]     = 0U;
	ipv4_hdr->id[1]     = 0U;
	ipv4_hdr->offset[0] = 0U;
	ipv4_hdr->offset[1] = 0U;

	ipv4_hdr->ttl       = net_pkt_ipv4_ttl(pkt);
	if (ipv4_hdr->ttl == 0U) {
		ipv4_hdr->ttl = net_if_ipv4_get_ttl(net_pkt_iface(pkt));
	}

	ipv4_hdr->proto     = 0U;
	ipv4_hdr->chksum    = 0U;

	net_ipaddr_copy(&ipv4_hdr->dst, dst);
	net_ipaddr_copy(&ipv4_hdr->src, src);

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	return net_pkt_set_data(pkt, &ipv4_access);
}

int net_ipv4_finalize(struct net_pkt *pkt, uint8_t next_header_proto)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	struct net_ipv4_hdr *ipv4_hdr;

	net_pkt_set_overwrite(pkt, true);

	ipv4_hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &ipv4_access);
	if (!ipv4_hdr) {
		return -ENOBUFS;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4_HDR_OPTIONS)) {
		if (net_pkt_ipv4_opts_len(pkt)) {
			ipv4_hdr->vhl = 0x40 | (0x0F &
					((net_pkt_ip_hdr_len(pkt) +
					  net_pkt_ipv4_opts_len(pkt)) / 4U));
		}
	}

	ipv4_hdr->len   = htons(net_pkt_get_len(pkt));
	ipv4_hdr->proto = next_header_proto;

	if (net_if_need_calc_tx_checksum(net_pkt_iface(pkt))) {
		ipv4_hdr->chksum = net_calc_chksum_ipv4(pkt);
	}

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

	return 0;
}

#if defined(CONFIG_NET_IPV4_HDR_OPTIONS)
int net_ipv4_parse_hdr_options(struct net_pkt *pkt,
			       net_ipv4_parse_hdr_options_cb_t cb,
			       void *user_data)
{
	struct net_pkt_cursor cur;
	uint8_t opt_data[NET_IPV4_HDR_OPTNS_MAX_LEN];
	uint8_t total_opts_len;

	if (!cb) {
		return -EINVAL;
	}

	net_pkt_cursor_backup(pkt, &cur);
	net_pkt_cursor_init(pkt);

	if (net_pkt_skip(pkt, sizeof(struct net_ipv4_hdr))) {
		return -EINVAL;
	}

	total_opts_len = net_pkt_ipv4_opts_len(pkt);

	while (total_opts_len) {
		uint8_t opt_len = 0U;
		uint8_t opt_type;

		if (net_pkt_read_u8(pkt, &opt_type)) {
			return -EINVAL;
		}

		total_opts_len--;

		if (!(opt_type == NET_IPV4_OPTS_EO ||
		      opt_type == NET_IPV4_OPTS_NOP)) {
			if (net_pkt_read_u8(pkt, &opt_len)) {
				return -EINVAL;
			}

			if (opt_len < 2U || total_opts_len < 1U) {
				return -EINVAL;
			}

			opt_len -= 2U;
			total_opts_len--;
		}

		if (opt_len > total_opts_len) {
			return -EINVAL;
		}

		switch (opt_type) {
		case NET_IPV4_OPTS_NOP:
			break;

		case NET_IPV4_OPTS_EO:
			/* Options length should be zero, when cursor reachs to
			 * End of options.
			 */
			if (total_opts_len) {
				return -EINVAL;
			}

			break;
		case NET_IPV4_OPTS_RR:
		case NET_IPV4_OPTS_TS:
			if (net_pkt_read(pkt, opt_data, opt_len)) {
				return -EINVAL;
			}

			if (cb(opt_type, opt_data, opt_len, user_data)) {
				return -EINVAL;
			}

			break;
		default:
			if (net_pkt_skip(pkt, opt_len)) {
				return -EINVAL;
			}

			break;
		}

		total_opts_len -= opt_len;
	}

	net_pkt_cursor_restore(pkt, &cur);

	return 0;
}
#endif

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
	uint8_t hdr_len;
	uint8_t opts_len;
	int pkt_len;

	net_stats_update_ipv4_recv(net_pkt_iface(pkt));

	hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &ipv4_access);
	if (!hdr) {
		NET_DBG("DROP: no buffer");
		goto drop;
	}

	hdr_len = (hdr->vhl & NET_IPV4_IHL_MASK) * 4U;
	if (hdr_len < sizeof(struct net_ipv4_hdr)) {
		NET_DBG("DROP: Invalid hdr length");
		goto drop;
	}

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	opts_len = hdr_len - sizeof(struct net_ipv4_hdr);
	if (opts_len > NET_IPV4_HDR_OPTNS_MAX_LEN) {
		return -EINVAL;
	}

	net_pkt_set_ipv4_opts_len(pkt, opts_len);

	pkt_len = ntohs(hdr->len);
	if (real_len < pkt_len) {
		NET_DBG("DROP: pkt len per hdr %d != pkt real len %d",
			pkt_len, real_len);
		goto drop;
	} else if (real_len > pkt_len) {
		net_pkt_update_length(pkt, pkt_len);
	}

	if (net_ipv4_is_addr_mcast(&hdr->src)) {
		NET_DBG("DROP: src addr is %s", "mcast");
		goto drop;
	}

	if (net_ipv4_is_addr_bcast(net_pkt_iface(pkt), &hdr->src)) {
		NET_DBG("DROP: src addr is %s", "bcast");
		goto drop;
	}

	if (net_ipv4_is_addr_unspecified(&hdr->src)) {
		NET_DBG("DROP: src addr is %s", "unspecified");
		goto drop;
	}

	if (net_if_need_calc_rx_checksum(net_pkt_iface(pkt)) &&
	    net_calc_chksum_ipv4(pkt) != 0U) {
		NET_DBG("DROP: invalid chksum");
		goto drop;
	}

	if ((!net_ipv4_is_my_addr(&hdr->dst) &&
	     !net_ipv4_is_addr_mcast(&hdr->dst) &&
	     !(hdr->proto == IPPROTO_UDP &&
	       (net_ipv4_addr_cmp(&hdr->dst, net_ipv4_broadcast_address()) ||
		/* RFC 1122 ch. 3.3.6 The 0.0.0.0 is non-standard bcast addr */
		(IS_ENABLED(CONFIG_NET_IPV4_ACCEPT_ZERO_BROADCAST) &&
		 net_ipv4_addr_cmp(&hdr->dst,
				   net_ipv4_unspecified_address()))))) ||
	    (hdr->proto == IPPROTO_TCP &&
	     net_ipv4_is_addr_bcast(net_pkt_iface(pkt), &hdr->dst))) {
		NET_DBG("DROP: not for me");
		goto drop;
	}

	net_pkt_acknowledge_data(pkt, &ipv4_access);

	if (opts_len) {
		/* Only few options are handled in EchoRequest, rest skipped */
		if (net_pkt_skip(pkt, opts_len)) {
			NET_DBG("Header too big? %u", hdr_len);
			goto drop;
		}
	}

	net_pkt_set_ipv4_ttl(pkt, hdr->ttl);

	net_pkt_set_family(pkt, PF_INET);

	NET_DBG("IPv4 packet received from %s to %s",
		log_strdup(net_sprint_ipv4_addr(&hdr->src)),
		log_strdup(net_sprint_ipv4_addr(&hdr->dst)));

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
