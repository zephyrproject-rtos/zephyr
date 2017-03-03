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
#include <net/nbuf.h>
#include <net/net_if.h>
#include "net_private.h"
#include "icmpv6.h"
#include "ipv6.h"
#include "net_stats.h"

#if defined(CONFIG_NET_RPL)
#include "rpl.h"
#endif

static sys_slist_t handlers;

void net_icmpv6_register_handler(struct net_icmpv6_handler *handler)
{
	sys_slist_prepend(&handlers, &handler->node);
}

void net_icmpv6_unregister_handler(struct net_icmpv6_handler *handler)
{
	sys_slist_find_and_remove(&handlers, &handler->node);
}

static inline void setup_ipv6_header(struct net_buf *buf, uint16_t extra_len,
				     uint8_t hop_limit, uint8_t icmp_type,
				     uint8_t icmp_code)
{
	NET_IPV6_BUF(buf)->vtc = 0x60;
	NET_IPV6_BUF(buf)->tcflow = 0;
	NET_IPV6_BUF(buf)->flow = 0;

	sys_put_be16(NET_ICMPH_LEN + extra_len + NET_ICMPV6_UNUSED_LEN,
		     NET_IPV6_BUF(buf)->len);

	NET_IPV6_BUF(buf)->nexthdr = IPPROTO_ICMPV6;
	NET_IPV6_BUF(buf)->hop_limit = hop_limit;

	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));

	NET_ICMP_BUF(buf)->type = icmp_type;
	NET_ICMP_BUF(buf)->code = icmp_code;

	/* ICMPv6 header has 4 unused bytes that must be zero, RFC 4443 ch 3.1
	 */
	memset(net_nbuf_icmp_data(buf) + sizeof(struct net_icmp_hdr), 0,
	       NET_ICMPV6_UNUSED_LEN);
}

#if defined(CONFIG_NET_DEBUG_ICMPV6)
static inline void echo_request_debug(struct net_buf *buf)
{
	char out[NET_IPV6_ADDR_LEN];

	snprintk(out, sizeof(out), "%s",
		 net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->dst));
	NET_DBG("Received Echo Request from %s to %s",
		net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->src), out);
}

static inline void echo_reply_debug(struct net_buf *buf)
{
	char out[NET_IPV6_ADDR_LEN];

	snprintk(out, sizeof(out), "%s",
		 net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->dst));
	NET_DBG("Sending Echo Reply from %s to %s",
		net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->src), out);
}
#else
#define echo_request_debug(buf)
#define echo_reply_debug(buf)
#endif /* CONFIG_NET_DEBUG_ICMPV6 */

static enum net_verdict handle_echo_request(struct net_buf *orig)
{
	struct net_buf *buf;
	struct net_buf *frag;
	struct net_if *iface;
	uint16_t payload_len;

	echo_request_debug(orig);

	iface = net_nbuf_iface(orig);

	buf = net_nbuf_get_reserve_tx(0, K_FOREVER);

	payload_len = sys_get_be16(NET_IPV6_BUF(orig)->len) -
		sizeof(NET_ICMPH_LEN) - NET_ICMPV6_UNUSED_LEN;

	frag = net_nbuf_copy_all(orig, 0, K_FOREVER);
	if (!frag) {
		goto drop;
	}

	net_buf_frag_add(buf, frag);
	net_nbuf_set_family(buf, AF_INET6);
	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_ll_reserve(buf, net_buf_headroom(frag));
	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));

	if (net_nbuf_ext_len(orig)) {
		net_nbuf_set_ext_len(buf, net_nbuf_ext_len(orig));
	} else {
		net_nbuf_set_ext_len(buf, 0);
	}

	/* Set up IPv6 Header fields */
	NET_IPV6_BUF(buf)->vtc = 0x60;
	NET_IPV6_BUF(buf)->tcflow = 0;
	NET_IPV6_BUF(buf)->flow = 0;
	NET_IPV6_BUF(buf)->hop_limit = net_if_ipv6_get_hop_limit(iface);

	/* ICMPv6 fields */
	NET_ICMP_BUF(buf)->type = NET_ICMPV6_ECHO_REPLY;
	NET_ICMP_BUF(buf)->code = 0;
	NET_ICMP_BUF(buf)->chksum = 0;

	if (net_is_ipv6_addr_mcast(&NET_IPV6_BUF(buf)->dst)) {
		net_ipaddr_copy(&NET_IPV6_BUF(buf)->dst,
				&NET_IPV6_BUF(orig)->src);

		net_ipaddr_copy(&NET_IPV6_BUF(buf)->src,
				net_if_ipv6_select_src_addr(iface,
						    &NET_IPV6_BUF(orig)->dst));
	} else {
		struct in6_addr addr;

		net_ipaddr_copy(&addr, &NET_IPV6_BUF(orig)->src);
		net_ipaddr_copy(&NET_IPV6_BUF(buf)->src,
				&NET_IPV6_BUF(orig)->dst);
		net_ipaddr_copy(&NET_IPV6_BUF(buf)->dst, &addr);
	}

	if (NET_IPV6_BUF(buf)->nexthdr == NET_IPV6_NEXTHDR_HBHO) {
#if defined(CONFIG_NET_RPL)
		uint16_t offset = NET_IPV6H_LEN;

		if (net_rpl_revert_header(buf, offset, &offset) < 0) {
			/* TODO: Handle error cases */
			goto drop;
		}
#endif
	}

	net_nbuf_ll_src(buf)->addr = net_nbuf_ll_dst(orig)->addr;
	net_nbuf_ll_src(buf)->len = net_nbuf_ll_dst(orig)->len;

	/* We must not set the destination ll address here but trust
	 * that it is set properly using a value from neighbor cache.
	 */
	net_nbuf_ll_dst(buf)->addr = NULL;

	NET_ICMP_BUF(buf)->chksum = 0;
	NET_ICMP_BUF(buf)->chksum = ~net_calc_chksum_icmpv6(buf);

	echo_reply_debug(buf);

	if (net_send_data(buf) < 0) {
		goto drop;
	}

	net_nbuf_unref(orig);
	net_stats_update_icmp_sent();

	return NET_OK;

drop:
	net_nbuf_unref(buf);
	net_stats_update_icmp_drop();

	return NET_DROP;
}

int net_icmpv6_send_error(struct net_buf *orig, uint8_t type, uint8_t code,
			  uint32_t param)
{
	struct net_buf *buf, *frag;
	struct net_if *iface;
	struct in6_addr *src, *dst;
	size_t extra_len, reserve;

	if (NET_IPV6_BUF(orig)->nexthdr == IPPROTO_ICMPV6) {
		if (NET_ICMP_BUF(orig)->code < 128) {
			/* We must not send ICMP errors back */
			return -EINVAL;
		}
	}

	iface = net_nbuf_iface(orig);

	buf = net_nbuf_get_reserve_tx(0, K_FOREVER);

	/* We need to remember the original location of source and destination
	 * addresses as the net_nbuf_copy() will mangle the original buffer.
	 */
	src = &NET_IPV6_BUF(orig)->src;
	dst = &NET_IPV6_BUF(orig)->dst;

	/* There is unsed part in ICMPv6 error msg header what we might need
	 * to store the param variable.
	 */
	reserve = sizeof(struct net_ipv6_hdr) + sizeof(struct net_icmp_hdr) +
		NET_ICMPV6_UNUSED_LEN;

	if (NET_IPV6_BUF(orig)->nexthdr == IPPROTO_UDP) {
		extra_len = sizeof(struct net_ipv6_hdr) +
			sizeof(struct net_udp_hdr);
	} else if (NET_IPV6_BUF(orig)->nexthdr == IPPROTO_TCP) {
		extra_len = sizeof(struct net_ipv6_hdr) +
			sizeof(struct net_tcp_hdr);
	} else {
		size_t space = CONFIG_NET_NBUF_DATA_SIZE -
			net_if_get_ll_reserve(iface, dst);

		if (reserve > space) {
			extra_len = 0;
		} else {
			extra_len = space - reserve;
		}
	}

	/* We only copy minimal IPv6 + next header from original message.
	 * This is so that the memory pressure is minimized.
	 */
	frag = net_nbuf_copy(orig, extra_len, reserve, K_FOREVER);
	if (!frag) {
		goto drop;
	}

	net_buf_frag_add(buf, frag);
	net_nbuf_set_family(buf, AF_INET6);
	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_ll_reserve(buf, net_buf_headroom(frag));
	net_nbuf_set_ext_len(buf, 0);

	setup_ipv6_header(buf, extra_len, net_if_ipv6_get_hop_limit(iface),
			  type, code);

	/* Depending on error option, we store the param into the ICMP message.
	 */
	if (type == NET_ICMPV6_PARAM_PROBLEM) {
		sys_put_be32(param, net_nbuf_icmp_data(buf) +
			     sizeof(struct net_icmp_hdr));
	}

	if (net_is_ipv6_addr_mcast(dst)) {
		net_ipaddr_copy(&NET_IPV6_BUF(buf)->dst, src);

		net_ipaddr_copy(&NET_IPV6_BUF(buf)->src,
				net_if_ipv6_select_src_addr(iface, dst));
	} else {
		struct in6_addr addr;

		net_ipaddr_copy(&addr, src);
		net_ipaddr_copy(&NET_IPV6_BUF(buf)->src, dst);
		net_ipaddr_copy(&NET_IPV6_BUF(buf)->dst, &addr);
	}

	net_nbuf_ll_src(buf)->addr = net_nbuf_ll_dst(orig)->addr;
	net_nbuf_ll_src(buf)->len = net_nbuf_ll_dst(orig)->len;
	net_nbuf_ll_dst(buf)->addr = net_nbuf_ll_src(orig)->addr;
	net_nbuf_ll_dst(buf)->len = net_nbuf_ll_src(orig)->len;

	NET_ICMP_BUF(buf)->chksum = 0;
	NET_ICMP_BUF(buf)->chksum = ~net_calc_chksum_icmpv6(buf);

#if defined(CONFIG_NET_DEBUG_ICMPV6)
	do {
		char out[NET_IPV6_ADDR_LEN];
		snprintk(out, sizeof(out), "%s",
			 net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->dst));
		NET_DBG("Sending ICMPv6 Error Message type %d code %d param %d"
			" from %s to %s", type, code, param,
			net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->src), out);
	} while (0);
#endif /* CONFIG_NET_DEBUG_ICMPV6 */

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

int net_icmpv6_send_echo_request(struct net_if *iface,
				 struct in6_addr *dst,
				 uint16_t identifier,
				 uint16_t sequence)
{
	const struct in6_addr *src;
	struct net_buf *buf;

	src = net_if_ipv6_select_src_addr(iface, dst);

	buf = net_nbuf_get_reserve_tx(net_if_get_ll_reserve(iface, dst),
				      K_FOREVER);

	buf = net_ipv6_create_raw(buf, src, dst, iface, IPPROTO_ICMPV6);

	net_nbuf_set_family(buf, AF_INET6);
	net_nbuf_set_iface(buf, iface);

	net_nbuf_append_u8(buf, NET_ICMPV6_ECHO_REQUEST);
	net_nbuf_append_u8(buf, 0);   /* code */
	net_nbuf_append_be16(buf, 0); /* checksum */
	net_nbuf_append_be16(buf, identifier);
	net_nbuf_append_be16(buf, sequence);

	net_ipaddr_copy(&NET_IPV6_BUF(buf)->src, src);
	net_ipaddr_copy(&NET_IPV6_BUF(buf)->dst, dst);

	NET_ICMP_BUF(buf)->chksum = 0;
	NET_ICMP_BUF(buf)->chksum = ~net_calc_chksum_icmpv6(buf);

	buf = net_ipv6_finalize_raw(buf, IPPROTO_ICMPV6);

#if defined(CONFIG_NET_DEBUG_ICMPV6)
	do {
		char out[NET_IPV6_ADDR_LEN];

		snprintk(out, sizeof(out), "%s",
			 net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->dst));
		NET_DBG("Sending ICMPv6 Echo Request type %d"
			" from %s to %s", NET_ICMPV6_ECHO_REQUEST,
			net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->src), out);
	} while (0);
#endif /* CONFIG_NET_DEBUG_ICMPV6 */

	if (net_send_data(buf) >= 0) {
		net_stats_update_icmp_sent();
		return 0;
	}

	net_nbuf_unref(buf);
	net_stats_update_icmp_drop();

	return -EIO;
}

enum net_verdict net_icmpv6_input(struct net_buf *buf,
				  uint8_t type, uint8_t code)
{
	struct net_icmpv6_handler *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&handlers, cb, node) {
		if (cb->type == type && (cb->code == code || cb->code == 0)) {
			net_stats_update_icmp_recv();
			return cb->handler(buf);
		}
	}

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
