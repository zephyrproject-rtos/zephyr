/** @file
 * @brief ICMPv4 related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_ICMPV4)
#define SYS_LOG_DOMAIN "net/icmpv4"
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>
#include <misc/slist.h>
#include <net/net_core.h>
#include <net/nbuf.h>
#include <net/net_if.h>
#include "net_private.h"
#include "icmpv4.h"
#include "net_stats.h"

static inline enum net_verdict handle_echo_request(struct net_buf *buf)
{
	/* Note that we send the same data buffers back and just swap
	 * the addresses etc.
	 */
	struct in_addr addr;

#if defined(CONFIG_NET_DEBUG_ICMPV4)
	char out[sizeof("xxx.xxx.xxx.xxx")];

	snprintk(out, sizeof(out), "%s",
		 net_sprint_ipv4_addr(&NET_IPV4_BUF(buf)->dst));
	NET_DBG("Received Echo Request from %s to %s",
		net_sprint_ipv4_addr(&NET_IPV4_BUF(buf)->src), out);
#endif /* CONFIG_NET_DEBUG_ICMPV4 */

	net_ipaddr_copy(&addr, &NET_IPV4_BUF(buf)->src);
	net_ipaddr_copy(&NET_IPV4_BUF(buf)->src,
			&NET_IPV4_BUF(buf)->dst);
	net_ipaddr_copy(&NET_IPV4_BUF(buf)->dst, &addr);

	NET_ICMP_BUF(buf)->type = NET_ICMPV4_ECHO_REPLY;
	NET_ICMP_BUF(buf)->code = 0;
	NET_ICMP_BUF(buf)->chksum = 0;
	NET_ICMP_BUF(buf)->chksum = ~net_calc_chksum_icmpv4(buf);

#if defined(CONFIG_NET_DEBUG_ICMPV4)
	snprintk(out, sizeof(out), "%s",
		 net_sprint_ipv4_addr(&NET_IPV4_BUF(buf)->dst));
	NET_DBG("Sending Echo Reply from %s to %s",
		net_sprint_ipv4_addr(&NET_IPV4_BUF(buf)->src), out);
#endif /* CONFIG_NET_DEBUG_ICMPV4 */

	if (net_send_data(buf) < 0) {
		net_stats_update_icmp_drop();
		return NET_DROP;
	}

	net_stats_update_icmp_sent();

	return NET_OK;
}

#define NET_ICMPV4_UNUSED_LEN 4

static inline void setup_ipv4_header(struct net_buf *buf, uint8_t extra_len,
				     uint8_t ttl, uint8_t icmp_type,
				     uint8_t icmp_code)
{
	NET_IPV4_BUF(buf)->vhl = 0x45;
	NET_IPV4_BUF(buf)->tos = 0x00;
	NET_IPV4_BUF(buf)->len[0] = 0;
	NET_IPV4_BUF(buf)->len[1] = sizeof(struct net_ipv4_hdr) +
		NET_ICMPH_LEN + extra_len + NET_ICMPV4_UNUSED_LEN;

	NET_IPV4_BUF(buf)->proto = IPPROTO_ICMP;
	NET_IPV4_BUF(buf)->ttl = ttl;
	NET_IPV4_BUF(buf)->offset[0] = NET_IPV4_BUF(buf)->offset[1] = 0;
	NET_IPV4_BUF(buf)->id[0] = NET_IPV4_BUF(buf)->id[1] = 0;

	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv4_hdr));

	NET_IPV4_BUF(buf)->chksum = 0;
	NET_IPV4_BUF(buf)->chksum = ~net_calc_chksum_ipv4(buf);

	NET_ICMP_BUF(buf)->type = icmp_type;
	NET_ICMP_BUF(buf)->code = icmp_code;

	memset(net_nbuf_icmp_data(buf) + sizeof(struct net_icmp_hdr), 0,
	       NET_ICMPV4_UNUSED_LEN);
}

int net_icmpv4_send_echo_request(struct net_if *iface,
				 struct in_addr *dst,
				 uint16_t identifier,
				 uint16_t sequence)
{
	const struct in_addr *src;
	struct net_buf *buf, *frag;
	uint16_t reserve;

	/* Take the first address of the network interface */
	src = &iface->ipv4.unicast[0].address.in_addr;

	buf = net_nbuf_get_reserve_tx(0);

	/* We cast to IPv6 address but that should be ok in this case
	 * as IPv4 cannot be used in 802.15.4 where it is the reserve
	 * size can change depending on address.
	 */
	reserve = net_if_get_ll_reserve(iface,
					(const struct in6_addr *)dst);

	frag = net_nbuf_get_reserve_data(reserve);

	net_buf_frag_add(buf, frag);
	net_nbuf_set_family(buf, AF_INET);
	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_ll_reserve(buf, reserve);

	setup_ipv4_header(buf, 0, net_if_ipv4_get_ttl(iface),
			  NET_ICMPV4_ECHO_REQUEST, 0);

	net_ipaddr_copy(&NET_IPV4_BUF(buf)->src, src);
	net_ipaddr_copy(&NET_IPV4_BUF(buf)->dst, dst);

	NET_ICMPV4_ECHO_REQ_BUF(buf)->identifier = htons(identifier);
	NET_ICMPV4_ECHO_REQ_BUF(buf)->sequence = htons(sequence);

	NET_ICMP_BUF(buf)->chksum = 0;
	NET_ICMP_BUF(buf)->chksum = ~net_calc_chksum_icmpv4(buf);

#if defined(CONFIG_NET_DEBUG_ICMPV4)
	do {
		char out[NET_IPV4_ADDR_LEN];

		snprintk(out, sizeof(out), "%s",
			 net_sprint_ipv4_addr(&NET_IPV4_BUF(buf)->dst));

		NET_DBG("Sending ICMPv4 Echo Request type %d"
			" from %s to %s", NET_ICMPV4_ECHO_REQUEST,
			net_sprint_ipv4_addr(&NET_IPV4_BUF(buf)->src), out);
	} while (0);
#endif /* CONFIG_NET_DEBUG_ICMPV4 */

	net_buf_add(buf->frags, sizeof(struct net_ipv4_hdr) +
		    sizeof(struct net_icmp_hdr) +
		    sizeof(struct net_icmpv4_echo_req));

	if (net_send_data(buf) >= 0) {
		net_stats_update_icmp_sent();
		return 0;
	}

	net_nbuf_unref(buf);
	net_stats_update_icmp_drop();

	return -EIO;
}

enum net_verdict net_icmpv4_input(struct net_buf *buf, uint16_t len,
				  uint8_t type, uint8_t code)
{
	ARG_UNUSED(code);
	ARG_UNUSED(len);

	switch (type) {
	case NET_ICMPV4_ECHO_REQUEST:
		return handle_echo_request(buf);
	}

	return NET_DROP;
}

int net_icmpv4_send_error(struct net_buf *orig, uint8_t type, uint8_t code)
{
	struct net_buf *buf, *frag;
	struct net_if *iface = net_nbuf_iface(orig);
	size_t extra_len, reserve;
	struct in_addr addr, *src, *dst;

	if (NET_IPV4_BUF(orig)->proto == IPPROTO_ICMP) {
		if (NET_ICMP_BUF(orig)->code < 8) {
			/* We must not send ICMP errors back */
			return -EINVAL;
		}
	}

	iface = net_nbuf_iface(orig);

	buf = net_nbuf_get_reserve_tx(0);

	reserve = sizeof(struct net_ipv4_hdr) + sizeof(struct net_icmp_hdr) +
		NET_ICMPV4_UNUSED_LEN;

	if (NET_IPV4_BUF(orig)->proto == IPPROTO_UDP) {
		extra_len = sizeof(struct net_ipv4_hdr) +
			sizeof(struct net_udp_hdr);
	} else if (NET_IPV4_BUF(orig)->proto == IPPROTO_TCP) {
		extra_len = sizeof(struct net_ipv4_hdr);
		/* FIXME, add TCP header length too */
	} else {
		size_t space = CONFIG_NET_NBUF_DATA_SIZE -
			net_if_get_ll_reserve(iface, NULL);

		if (reserve > space) {
			extra_len = 0;
		} else {
			extra_len = space - reserve;
		}
	}

	/* We need to remember the original location of source and destination
	 * addresses as the net_nbuf_copy() will mangle the original buffer.
	 */
	src = &NET_IPV4_BUF(orig)->src;
	dst = &NET_IPV4_BUF(orig)->dst;

	/* We only copy minimal IPv4 + next header from original message.
	 * This is so that the memory pressure is minimized.
	 */
	frag = net_nbuf_copy(orig->frags, extra_len, reserve);
	if (!frag) {
		goto drop;
	}

	net_buf_frag_add(buf, frag);
	net_nbuf_set_family(buf, AF_INET);
	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_ll_reserve(buf, net_buf_headroom(frag));

	setup_ipv4_header(buf, extra_len, net_if_ipv4_get_ttl(iface),
			  type, code);

	net_ipaddr_copy(&addr, src);
	net_ipaddr_copy(&NET_IPV4_BUF(buf)->src, dst);
	net_ipaddr_copy(&NET_IPV4_BUF(buf)->dst, &addr);

	net_nbuf_ll_src(buf)->addr = net_nbuf_ll_dst(orig)->addr;
	net_nbuf_ll_src(buf)->len = net_nbuf_ll_dst(orig)->len;
	net_nbuf_ll_dst(buf)->addr = net_nbuf_ll_src(orig)->addr;
	net_nbuf_ll_dst(buf)->len = net_nbuf_ll_src(orig)->len;

	NET_ICMP_BUF(buf)->chksum = 0;
	NET_ICMP_BUF(buf)->chksum = ~net_calc_chksum_icmpv4(buf);

#if defined(CONFIG_NET_DEBUG_ICMPV4)
	do {
		char out[sizeof("xxx.xxx.xxx.xxx")];

		snprintk(out, sizeof(out), "%s",
			 net_sprint_ipv4_addr(&NET_IPV4_BUF(buf)->dst));
		NET_DBG("Sending ICMPv4 Error Message type %d code %d "
			"from %s to %s", type, code,
			net_sprint_ipv4_addr(&NET_IPV4_BUF(buf)->src), out);
	} while (0);
#endif /* CONFIG_NET_DEBUG_ICMPV4 */

	if (net_send_data(buf) >= 0) {
		net_stats_update_icmp_sent();
		return -EIO;
	}

drop:
	net_nbuf_unref(buf);
	net_stats_update_icmp_drop();

	/* Note that we always return < 0 so that the caller knows to
	 * discard the original buffer.
	 */
	return -EIO;
}
