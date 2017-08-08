/** @file
 * @brief ICMPv6 related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_ICMPV6)
#define SYS_LOG_DOMAIN "net/icmpv6"
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>
#include <misc/slist.h>
#include <misc/byteorder.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include "net_private.h"
#include "icmpv6.h"
#include "ipv6.h"
#include "net_stats.h"

#if defined(CONFIG_NET_RPL)
#include "rpl.h"
#endif

#define PKT_WAIT_TIME K_SECONDS(1)

static sys_slist_t handlers;

const char *net_icmpv6_type2str(int icmpv6_type)
{
	switch (icmpv6_type) {
	case NET_ICMPV6_DST_UNREACH:
		return "Destination Unreachable";
	case NET_ICMPV6_PACKET_TOO_BIG:
		return "Packet Too Big";
	case NET_ICMPV6_TIME_EXCEEDED:
		return "Time Exceeded";
	case NET_ICMPV6_PARAM_PROBLEM:
		return "IPv6 Bad Header";
	case NET_ICMPV6_ECHO_REQUEST:
		return "Echo Request";
	case NET_ICMPV6_ECHO_REPLY:
		return "Echo Reply";
	case NET_ICMPV6_MLD_QUERY:
		return "Multicast Listener Query";
	case NET_ICMPV6_RS:
		return "Router Solicitation";
	case NET_ICMPV6_RA:
		return "Router Advertisement";
	case NET_ICMPV6_NS:
		return "Neighbor Solicitation";
	case NET_ICMPV6_NA:
		return "Neighbor Advertisement";
	case NET_ICMPV6_MLDv2:
		return "Multicast Listener Report v2";
	}

	return "?";
}

void net_icmpv6_register_handler(struct net_icmpv6_handler *handler)
{
	sys_slist_prepend(&handlers, &handler->node);
}

void net_icmpv6_unregister_handler(struct net_icmpv6_handler *handler)
{
	sys_slist_find_and_remove(&handlers, &handler->node);
}

static inline void setup_ipv6_header(struct net_pkt *pkt, u16_t extra_len,
				     u8_t hop_limit, u8_t icmp_type,
				     u8_t icmp_code)
{
	struct net_buf *frag = pkt->frags;
	const u32_t unused = 0;
	u16_t pos;

	NET_IPV6_HDR(pkt)->vtc = 0x60;
	NET_IPV6_HDR(pkt)->tcflow = 0;
	NET_IPV6_HDR(pkt)->flow = 0;

	sys_put_be16(NET_ICMPH_LEN + extra_len + NET_ICMPV6_UNUSED_LEN,
		     NET_IPV6_HDR(pkt)->len);

	NET_IPV6_HDR(pkt)->nexthdr = IPPROTO_ICMPV6;
	NET_IPV6_HDR(pkt)->hop_limit = hop_limit;

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));

	frag = net_pkt_write(pkt, frag, net_pkt_ip_hdr_len(pkt), &pos,
			     sizeof(icmp_type), &icmp_type, PKT_WAIT_TIME);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(icmp_code),
			     &icmp_code, PKT_WAIT_TIME);

	/* ICMPv6 header has 4 unused bytes that must be zero, RFC 4443 ch 3.1
	 */
	net_pkt_write(pkt, frag, pos, &pos, 4, (u8_t *)&unused, PKT_WAIT_TIME);
}

#if defined(CONFIG_NET_DEBUG_ICMPV6)
static inline void echo_request_debug(struct net_pkt *pkt)
{
	char out[NET_IPV6_ADDR_LEN];

	snprintk(out, sizeof(out), "%s",
		 net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->dst));
	NET_DBG("Received Echo Request from %s to %s",
		net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->src), out);
}

static inline void echo_reply_debug(struct net_pkt *pkt)
{
	char out[NET_IPV6_ADDR_LEN];

	snprintk(out, sizeof(out), "%s",
		 net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->dst));
	NET_DBG("Sending Echo Reply from %s to %s",
		net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->src), out);
}
#else
#define echo_request_debug(pkt)
#define echo_reply_debug(pkt)
#endif /* CONFIG_NET_DEBUG_ICMPV6 */

struct net_buf *net_icmpv6_set_chksum(struct net_pkt *pkt,
				      struct net_buf *frag)
{
	struct net_icmp_hdr *icmp_hdr;
	u16_t chksum = 0;
	u16_t pos;

	icmp_hdr = net_pkt_icmp_data(pkt);
	if (net_icmp_header_fits(pkt, icmp_hdr)) {
		icmp_hdr->chksum = 0;
		icmp_hdr->chksum = ~net_calc_chksum_icmpv6(pkt);

		return frag;
	}

	frag = net_pkt_write(pkt, frag, net_pkt_ip_hdr_len(pkt) +
			     net_pkt_ipv6_ext_len(pkt) +
			     1 + 1 /* type + code */,
			     &pos, sizeof(chksum), (u8_t *)&chksum,
			     PKT_WAIT_TIME);

	chksum = ~net_calc_chksum_icmpv6(pkt);

	frag = net_pkt_write(pkt, frag, pos - 2, &pos, sizeof(chksum),
			     (u8_t *)&chksum, PKT_WAIT_TIME);

	NET_ASSERT(frag);

	return frag;
}

struct net_icmp_hdr *net_icmpv6_get_hdr(struct net_pkt *pkt,
					struct net_icmp_hdr *hdr)
{
	struct net_icmp_hdr *icmp_hdr;
	struct net_buf *frag;
	u16_t pos;

	/* If the ICMP header can fit the first fragment, then access it
	 * directly (fast path), otherwise read the values one by one
	 * using net_frag_read*() functions (slow path).
	 */

	icmp_hdr = net_pkt_icmp_data(pkt);
	if (net_icmp_header_fits(pkt, icmp_hdr)) {
		return icmp_hdr;
	}

	frag = net_frag_read_u8(pkt->frags, net_pkt_ip_hdr_len(pkt) +
				net_pkt_ipv6_ext_len(pkt), &pos, &hdr->type);
	frag = net_frag_read_u8(frag, pos, &pos, &hdr->code);
	frag = net_frag_read(frag, pos, &pos, sizeof(hdr->chksum),
			     (u8_t *)&hdr->chksum);
	if (!frag) {
		NET_ASSERT(frag);
		return NULL;
	}

	return hdr;
}

struct net_icmp_hdr *net_icmpv6_set_hdr(struct net_pkt *pkt,
					struct net_icmp_hdr *hdr)
{
	struct net_buf *frag;
	u16_t pos;

	if (net_icmp_header_fits(pkt, hdr)) {
		return hdr;
	}

	frag = net_pkt_write(pkt, pkt->frags, net_pkt_ip_hdr_len(pkt) +
			     net_pkt_ipv6_ext_len(pkt), &pos,
			     sizeof(hdr->type), &hdr->type, PKT_WAIT_TIME);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(hdr->code),
			     &hdr->code, PKT_WAIT_TIME);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(hdr->chksum),
			     (u8_t *)&hdr->chksum, PKT_WAIT_TIME);
	if (!frag) {
		NET_ASSERT(frag);
		return NULL;
	}

	return hdr;
}

struct net_icmpv6_ns_hdr *net_icmpv6_get_ns_hdr(struct net_pkt *pkt,
						struct net_icmpv6_ns_hdr *hdr)
{
	struct net_buf *frag;
	u8_t *opt_data;
	u16_t pos;

	/* No helpers for various ICMP sub-options. First get the pointer
	 * where the sub-option is located, then check if it is located in
	 * first fragment (fast path). If the header is not in first fragment,
	 * then read the values separately (slow path).
	 */

	opt_data = net_pkt_icmp_opt_data(pkt, sizeof(struct net_icmp_hdr));
	if (net_header_fits(pkt, opt_data, sizeof(*hdr))) {
		return (struct net_icmpv6_ns_hdr *)opt_data;
	}

	frag = net_frag_read(pkt->frags,
			     net_pkt_ip_hdr_len(pkt) +
			     net_pkt_ipv6_ext_len(pkt) +
			     sizeof(struct net_icmp_hdr) + 4 /* reserved */,
			     &pos, sizeof(struct in6_addr), (u8_t *)&hdr->tgt);
	if (!frag) {
		NET_ASSERT(frag);
		return NULL;
	}

	return hdr;
}

struct net_icmpv6_ns_hdr *net_icmpv6_set_ns_hdr(struct net_pkt *pkt,
						struct net_icmpv6_ns_hdr *hdr)
{
	const u32_t reserved = 0;
	struct net_buf *frag;
	u8_t *opt_data;
	u16_t pos;

	opt_data = net_pkt_icmp_opt_data(pkt, sizeof(struct net_icmp_hdr));
	if (net_header_fits(pkt, opt_data, sizeof(*hdr))) {
		return (struct net_icmpv6_ns_hdr *)opt_data;
	}

	frag = net_pkt_write(pkt, pkt->frags, net_pkt_ip_hdr_len(pkt) +
			     net_pkt_ipv6_ext_len(pkt) +
			     sizeof(struct net_icmp_hdr), &pos,
			     sizeof(reserved), (u8_t *)&reserved,
			     PKT_WAIT_TIME);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(struct in6_addr),
			     (u8_t *)&hdr->tgt, PKT_WAIT_TIME);
	if (!frag) {
		NET_ASSERT(frag);
		return NULL;
	}

	return hdr;
}

struct net_icmpv6_nd_opt_hdr *net_icmpv6_get_nd_opt_hdr(struct net_pkt *pkt,
					    struct net_icmpv6_nd_opt_hdr *hdr)
{
	struct net_buf *frag;
	u8_t *opt_data;
	u16_t pos;

	opt_data = net_pkt_icmp_opt_data(pkt, sizeof(struct net_icmp_hdr) +
					 net_pkt_ipv6_ext_opt_len(pkt));
	if (net_header_fits(pkt, opt_data, sizeof(*hdr))) {
		return (struct net_icmpv6_nd_opt_hdr *)opt_data;
	}

	frag = net_frag_read_u8(pkt->frags,
				net_pkt_ip_hdr_len(pkt) +
				net_pkt_ipv6_ext_len(pkt) +
				sizeof(struct net_icmp_hdr) +
				net_pkt_ipv6_ext_opt_len(pkt),
				&pos, &hdr->type);
	frag = net_frag_read_u8(frag, pos, &pos, &hdr->len);
	if (!frag) {
		return NULL;
	}

	return hdr;
}

struct net_icmpv6_na_hdr *net_icmpv6_get_na_hdr(struct net_pkt *pkt,
						struct net_icmpv6_na_hdr *hdr)
{
	struct net_buf *frag;
	u8_t *opt_data;
	u16_t pos;

	opt_data = net_pkt_icmp_opt_data(pkt, sizeof(struct net_icmp_hdr));
	if (net_header_fits(pkt, opt_data, sizeof(*hdr))) {
		return (struct net_icmpv6_na_hdr *)opt_data;
	}

	frag = net_frag_read_u8(pkt->frags, net_pkt_ip_hdr_len(pkt) +
				net_pkt_ipv6_ext_len(pkt) +
				sizeof(struct net_icmp_hdr),
				&pos, &hdr->flags);
	frag = net_frag_skip(frag, pos, &pos, 3); /* reserved */
	frag = net_frag_read(frag, pos, &pos, sizeof(struct in6_addr),
			     (u8_t *)&hdr->tgt);
	if (!frag) {
		NET_ASSERT(frag);
		return NULL;
	}

	return hdr;
}

struct net_icmpv6_na_hdr *net_icmpv6_set_na_hdr(struct net_pkt *pkt,
						struct net_icmpv6_na_hdr *hdr)
{
	const u8_t reserved[3] = { 0 };
	struct net_buf *frag;
	u8_t *opt_data;
	u16_t pos;

	opt_data = net_pkt_icmp_opt_data(pkt, sizeof(struct net_icmp_hdr));
	if (net_header_fits(pkt, opt_data, sizeof(*hdr))) {
		return (struct net_icmpv6_na_hdr *)opt_data;
	}

	frag = net_pkt_write(pkt, pkt->frags,
			     net_pkt_ip_hdr_len(pkt) +
			     net_pkt_ipv6_ext_len(pkt) +
			     sizeof(struct net_icmp_hdr),
			     &pos, sizeof(hdr->flags), &hdr->flags,
			     PKT_WAIT_TIME);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(reserved),
			     (u8_t *)reserved, PKT_WAIT_TIME);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(struct in6_addr),
			     (u8_t *)&hdr->tgt, PKT_WAIT_TIME);
	if (!frag) {
		NET_ASSERT(frag);
		return NULL;
	}

	return hdr;
}

struct net_icmpv6_ra_hdr *net_icmpv6_get_ra_hdr(struct net_pkt *pkt,
						struct net_icmpv6_ra_hdr *hdr)
{
	struct net_buf *frag;
	u8_t *opt_data;
	u16_t pos;

	opt_data = net_pkt_icmp_opt_data(pkt, sizeof(struct net_icmp_hdr));
	if (net_header_fits(pkt, opt_data, sizeof(*hdr))) {
		return (struct net_icmpv6_ra_hdr *)opt_data;
	}

	frag = net_frag_read_u8(pkt->frags, net_pkt_ip_hdr_len(pkt) +
				net_pkt_ipv6_ext_len(pkt) +
				sizeof(struct net_icmp_hdr),
				&pos, &hdr->cur_hop_limit);
	frag = net_frag_read_u8(frag, pos, &pos, &hdr->flags);
	frag = net_frag_read(frag, pos, &pos, sizeof(hdr->router_lifetime),
			     (u8_t *)&hdr->router_lifetime);
	frag = net_frag_read(frag, pos, &pos, sizeof(hdr->reachable_time),
			     (u8_t *)&hdr->reachable_time);
	frag = net_frag_read(frag, pos, &pos, sizeof(hdr->retrans_timer),
			     (u8_t *)&hdr->retrans_timer);
	if (!frag) {
		NET_ASSERT(frag);
		return NULL;
	}

	return hdr;
}

static enum net_verdict handle_echo_request(struct net_pkt *orig)
{
	struct net_icmp_hdr hdr, *icmp_hdr;
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_if *iface;
	u16_t payload_len;

	echo_request_debug(orig);

	iface = net_pkt_iface(orig);

	pkt = net_pkt_get_reserve_tx(0, PKT_WAIT_TIME);
	if (!pkt) {
		goto drop_no_pkt;
	}

	payload_len = sys_get_be16(NET_IPV6_HDR(orig)->len) -
		sizeof(NET_ICMPH_LEN) - NET_ICMPV6_UNUSED_LEN;

	frag = net_pkt_copy_all(orig, 0, PKT_WAIT_TIME);
	if (!frag) {
		goto drop;
	}

	net_pkt_frag_add(pkt, frag);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_iface(pkt, iface);
	net_pkt_set_ll_reserve(pkt, net_buf_headroom(frag));
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));

	if (net_pkt_ipv6_ext_len(orig)) {
		net_pkt_set_ipv6_ext_len(pkt, net_pkt_ipv6_ext_len(orig));
	} else {
		net_pkt_set_ipv6_ext_len(pkt, 0);
	}

	/* Set up IPv6 Header fields */
	NET_IPV6_HDR(pkt)->vtc = 0x60;
	NET_IPV6_HDR(pkt)->tcflow = 0;
	NET_IPV6_HDR(pkt)->flow = 0;
	NET_IPV6_HDR(pkt)->hop_limit = net_if_ipv6_get_hop_limit(iface);

	if (net_is_ipv6_addr_mcast(&NET_IPV6_HDR(pkt)->dst)) {
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst,
				&NET_IPV6_HDR(orig)->src);

		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src,
				net_if_ipv6_select_src_addr(iface,
						    &NET_IPV6_HDR(orig)->dst));
	} else {
		struct in6_addr addr;

		net_ipaddr_copy(&addr, &NET_IPV6_HDR(orig)->src);
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src,
				&NET_IPV6_HDR(orig)->dst);
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst, &addr);
	}

	if (NET_IPV6_HDR(pkt)->nexthdr == NET_IPV6_NEXTHDR_HBHO) {
#if defined(CONFIG_NET_RPL)
		u16_t offset = NET_IPV6H_LEN;

		if (net_rpl_revert_header(pkt, offset, &offset) < 0) {
			/* TODO: Handle error cases */
			goto drop;
		}
#endif
	}

	net_pkt_ll_src(pkt)->addr = net_pkt_ll_dst(orig)->addr;
	net_pkt_ll_src(pkt)->len = net_pkt_ll_dst(orig)->len;

	/* We must not set the destination ll address here but trust
	 * that it is set properly using a value from neighbor cache.
	 */
	net_pkt_ll_dst(pkt)->addr = NULL;

	/* ICMPv6 fields */
	icmp_hdr = net_icmpv6_get_hdr(pkt, &hdr);
	icmp_hdr->type = NET_ICMPV6_ECHO_REPLY;
	icmp_hdr->code = 0;
	icmp_hdr->chksum = 0;
	net_icmpv6_set_hdr(pkt, icmp_hdr);
	net_icmpv6_set_chksum(pkt, pkt->frags);

	echo_reply_debug(pkt);

	if (net_send_data(pkt) < 0) {
		goto drop;
	}

	net_pkt_unref(orig);
	net_stats_update_icmp_sent();

	return NET_OK;

drop:
	net_pkt_unref(pkt);

drop_no_pkt:
	net_stats_update_icmp_drop();

	return NET_DROP;
}

int net_icmpv6_send_error(struct net_pkt *orig, u8_t type, u8_t code,
			  u32_t param)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_if *iface;
	size_t extra_len, reserve;
	int err = -EIO;

	if (NET_IPV6_HDR(orig)->nexthdr == IPPROTO_ICMPV6) {
		struct net_icmp_hdr icmp_hdr[1];

		if (!net_icmpv6_get_hdr(orig, icmp_hdr) ||
		    icmp_hdr->code < 128) {
			/* We must not send ICMP errors back */
			err = -EINVAL;
			goto drop_no_pkt;
		}
	}

	iface = net_pkt_iface(orig);

	pkt = net_pkt_get_reserve_tx(0, PKT_WAIT_TIME);
	if (!pkt) {
		err = -ENOMEM;
		goto drop_no_pkt;
	}

	/* There is unsed part in ICMPv6 error msg header what we might need
	 * to store the param variable.
	 */
	reserve = sizeof(struct net_ipv6_hdr) + sizeof(struct net_icmp_hdr) +
		NET_ICMPV6_UNUSED_LEN;

	if (NET_IPV6_HDR(orig)->nexthdr == IPPROTO_UDP) {
		extra_len = sizeof(struct net_ipv6_hdr) +
			sizeof(struct net_udp_hdr);
	} else if (NET_IPV6_HDR(orig)->nexthdr == IPPROTO_TCP) {
		extra_len = sizeof(struct net_ipv6_hdr) +
			sizeof(struct net_tcp_hdr);
	} else {
		size_t space = CONFIG_NET_BUF_DATA_SIZE -
			net_if_get_ll_reserve(iface,
					      &NET_IPV6_HDR(orig)->dst);

		if (reserve > space) {
			extra_len = 0;
		} else {
			extra_len = space - reserve;
		}
	}

	/* We only copy minimal IPv6 + next header from original message.
	 * This is so that the memory pressure is minimized.
	 */
	frag = net_pkt_copy(orig, extra_len, reserve, PKT_WAIT_TIME);
	if (!frag) {
		err = -ENOMEM;
		goto drop;
	}

	net_pkt_frag_add(pkt, frag);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_iface(pkt, iface);
	net_pkt_set_ll_reserve(pkt, net_buf_headroom(frag));
	net_pkt_set_ipv6_ext_len(pkt, 0);

	setup_ipv6_header(pkt, extra_len, net_if_ipv6_get_hop_limit(iface),
			  type, code);

	/* Depending on error option, we store the param into the ICMP message.
	 */
	if (type == NET_ICMPV6_PARAM_PROBLEM) {
		sys_put_be32(param, (u8_t *)net_pkt_icmp_data(pkt) +
			     sizeof(struct net_icmp_hdr));
	}

	if (net_is_ipv6_addr_mcast(&NET_IPV6_HDR(orig)->dst)) {
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst,
				&NET_IPV6_HDR(orig)->src);

		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src,
				net_if_ipv6_select_src_addr(iface,
						    &NET_IPV6_HDR(orig)->dst));
	} else {
		struct in6_addr addr;

		net_ipaddr_copy(&addr, &NET_IPV6_HDR(orig)->src);
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src,
				&NET_IPV6_HDR(orig)->dst);
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst, &addr);
	}

	net_pkt_ll_src(pkt)->addr = net_pkt_ll_dst(orig)->addr;
	net_pkt_ll_src(pkt)->len = net_pkt_ll_dst(orig)->len;
	net_pkt_ll_dst(pkt)->addr = net_pkt_ll_src(orig)->addr;
	net_pkt_ll_dst(pkt)->len = net_pkt_ll_src(orig)->len;

	/* Clear and then set the chksum */
	frag = net_icmpv6_set_chksum(pkt, pkt->frags);
	NET_ASSERT(frag);

#if defined(CONFIG_NET_DEBUG_ICMPV6)
	do {
		char out[NET_IPV6_ADDR_LEN];
		snprintk(out, sizeof(out), "%s",
			 net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->dst));
		NET_DBG("Sending ICMPv6 Error Message type %d code %d param %d"
			" from %s to %s", type, code, param,
			net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->src), out);
	} while (0);
#endif /* CONFIG_NET_DEBUG_ICMPV6 */

	if (net_send_data(pkt) >= 0) {
		net_stats_update_icmp_sent();
		return 0;
	}

drop:
	net_pkt_unref(pkt);

drop_no_pkt:
	net_stats_update_icmp_drop();

	return err;
}

int net_icmpv6_send_echo_request(struct net_if *iface,
				 struct in6_addr *dst,
				 u16_t identifier,
				 u16_t sequence)
{
	const struct in6_addr *src;
	struct net_pkt *pkt;

	src = net_if_ipv6_select_src_addr(iface, dst);

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface, dst),
				     K_FOREVER);

	pkt = net_ipv6_create_raw(pkt, src, dst, iface, IPPROTO_ICMPV6);

	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_iface(pkt, iface);

	net_pkt_append_u8(pkt, NET_ICMPV6_ECHO_REQUEST);
	net_pkt_append_u8(pkt, 0);   /* code */
	net_pkt_append_be16(pkt, 0); /* checksum */
	net_pkt_append_be16(pkt, identifier);
	net_pkt_append_be16(pkt, sequence);

	net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src, src);
	net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst, dst);

	/* Clear and then set the chksum */
	net_icmpv6_set_chksum(pkt, pkt->frags);

	if (net_ipv6_finalize_raw(pkt, IPPROTO_ICMPV6) < 0) {
		goto drop;
	}

#if defined(CONFIG_NET_DEBUG_ICMPV6)
	do {
		char out[NET_IPV6_ADDR_LEN];

		snprintk(out, sizeof(out), "%s",
			 net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->dst));
		NET_DBG("Sending ICMPv6 Echo Request type %d"
			" from %s to %s", NET_ICMPV6_ECHO_REQUEST,
			net_sprint_ipv6_addr(&NET_IPV6_HDR(pkt)->src), out);
	} while (0);
#endif /* CONFIG_NET_DEBUG_ICMPV6 */

	if (net_send_data(pkt) >= 0) {
		net_stats_update_icmp_sent();
		return 0;
	}

drop:
	net_pkt_unref(pkt);
	net_stats_update_icmp_drop();

	return -EIO;
}

enum net_verdict net_icmpv6_input(struct net_pkt *pkt,
				  u8_t type, u8_t code)
{
	struct net_icmpv6_handler *cb;

	net_stats_update_icmp_recv();

	SYS_SLIST_FOR_EACH_CONTAINER(&handlers, cb, node) {
		if (cb->type == type && (cb->code == code || cb->code == 0)) {
			return cb->handler(pkt);
		}
	}

	net_stats_update_icmp_drop();

	return NET_DROP;
}

static struct net_icmpv6_handler echo_request_handler = {
	.type = NET_ICMPV6_ECHO_REQUEST,
	.code = 0,
	.handler = handle_echo_request,
};

void net_icmpv6_init(void)
{
	net_icmpv6_register_handler(&echo_request_handler);
}
