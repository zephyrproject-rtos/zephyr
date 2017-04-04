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
#include <net/nbuf.h>
#include <net/net_stats.h>
#include <net/net_context.h>
#include "net_private.h"
#include "connection.h"
#include "net_stats.h"
#include "icmpv4.h"
#include "ipv4.h"

struct net_buf *net_ipv4_create_raw(struct net_buf *buf,
				    const struct in_addr *src,
				    const struct in_addr *dst,
				    struct net_if *iface,
				    uint8_t next_header)
{
	struct net_buf *header;

	header = net_nbuf_get_frag(buf, K_FOREVER);

	net_buf_frag_insert(buf, header);

	NET_IPV4_BUF(buf)->vhl = 0x45;
	NET_IPV4_BUF(buf)->tos = 0x00;
	NET_IPV4_BUF(buf)->proto = 0;

	NET_IPV4_BUF(buf)->ttl = net_if_ipv4_get_ttl(iface);
	NET_IPV4_BUF(buf)->offset[0] = NET_IPV4_BUF(buf)->offset[1] = 0;
	NET_IPV4_BUF(buf)->id[0] = NET_IPV4_BUF(buf)->id[1] = 0;

	net_ipaddr_copy(&NET_IPV4_BUF(buf)->dst, dst);
	net_ipaddr_copy(&NET_IPV4_BUF(buf)->src, src);

	NET_IPV4_BUF(buf)->proto = next_header;

	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv4_hdr));
	net_nbuf_set_family(buf, AF_INET);

	net_buf_add(header, sizeof(struct net_ipv4_hdr));

	return buf;
}

struct net_buf *net_ipv4_create(struct net_context *context,
				struct net_buf *buf,
				const struct in_addr *src,
				const struct in_addr *dst)
{
	NET_ASSERT(((struct sockaddr_in_ptr *)&context->local)->sin_addr);

	if (!src) {
		src = ((struct sockaddr_in_ptr *)&context->local)->sin_addr;
	}

	if (net_is_ipv4_addr_unspecified(src)
	    || net_is_ipv4_addr_mcast(src)) {
		src = &net_nbuf_iface(buf)->ipv4.unicast[0].address.in_addr;
	}

	return net_ipv4_create_raw(buf,
				   src,
				   dst,
				   net_context_get_iface(context),
				   net_context_get_ip_proto(context));
}

int net_ipv4_finalize_raw(struct net_buf *buf, uint8_t next_header)
{
	/* Set the length of the IPv4 header */
	size_t total_len;

	net_nbuf_compact(buf);

	total_len = net_buf_frags_len(buf->frags);

	NET_IPV4_BUF(buf)->len[0] = total_len / 256;
	NET_IPV4_BUF(buf)->len[1] = total_len - NET_IPV4_BUF(buf)->len[0] * 256;

	NET_IPV4_BUF(buf)->chksum = 0;
	NET_IPV4_BUF(buf)->chksum = ~net_calc_chksum_ipv4(buf);

#if defined(CONFIG_NET_UDP)
	if (next_header == IPPROTO_UDP) {
		NET_UDP_BUF(buf)->chksum = 0;
		NET_UDP_BUF(buf)->chksum = ~net_calc_chksum_udp(buf);
	}
#endif
#if defined(CONFIG_NET_TCP)
	if (next_header == IPPROTO_TCP) {
		NET_TCP_BUF(buf)->chksum = 0;
		NET_TCP_BUF(buf)->chksum = ~net_calc_chksum_tcp(buf);
	}
#endif

	return 0;
}

int net_ipv4_finalize(struct net_context *context, struct net_buf *buf)
{
	return net_ipv4_finalize_raw(buf,
				     net_context_get_ip_proto(context));
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

static inline enum net_verdict process_icmpv4_pkt(struct net_buf *buf,
						  struct net_ipv4_hdr *ipv4)
{
	struct net_icmp_hdr *hdr = NET_ICMP_BUF(buf);

	NET_DBG("ICMPv4 packet received type %d code %d",
		hdr->type, hdr->code);

	return net_icmpv4_input(buf, hdr->type, hdr->code);
}

enum net_verdict net_ipv4_process_pkt(struct net_buf *buf)
{
	struct net_ipv4_hdr *hdr = NET_IPV4_BUF(buf);
	int real_len = net_buf_frags_len(buf);
	int pkt_len = (hdr->len[0] << 8) + hdr->len[1];
	enum net_verdict verdict = NET_DROP;

	if (real_len != pkt_len) {
		NET_DBG("IPv4 packet size %d buf len %d", pkt_len, real_len);
		goto drop;
	}

#if defined(CONFIG_NET_DEBUG_IPV4)
	do {
		char out[sizeof("xxx.xxx.xxx.xxx")];

		snprintk(out, sizeof(out), "%s",
			 net_sprint_ipv4_addr(&hdr->dst));
		NET_DBG("IPv4 packet received from %s to %s",
			net_sprint_ipv4_addr(&hdr->src), out);
	} while (0);
#endif /* CONFIG_NET_DEBUG_IPV4 */

	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv4_hdr));

	if (!net_is_my_ipv4_addr(&hdr->dst)) {
#if defined(CONFIG_NET_DHCPV4)
		if (hdr->proto == IPPROTO_UDP &&
		    net_ipv4_addr_cmp(&hdr->dst,
				      net_ipv4_broadcast_address())) {

			verdict = net_conn_input(IPPROTO_UDP, buf);
			if (verdict != NET_DROP) {
				return verdict;
			}
		}
#endif
		NET_DBG("IPv4 packet in buf %p not for me", buf);
		goto drop;
	}

	switch (hdr->proto) {
	case IPPROTO_ICMP:
		verdict = process_icmpv4_pkt(buf, hdr);
		break;
	case IPPROTO_UDP:
		verdict = net_conn_input(IPPROTO_UDP, buf);
		break;
	case IPPROTO_TCP:
		verdict = net_conn_input(IPPROTO_TCP, buf);
		break;
	}

	if (verdict != NET_DROP) {
		return verdict;
	}

drop:
	net_stats_update_ipv4_drop();
	return NET_DROP;
}
