/** @file
 * @brief ICMPv6 related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef CONFIG_NET_DEBUG_ICMPV6
#define SYS_LOG_DOMAIN "net/icmpv6"
#define NET_DEBUG 1
#endif

#include <errno.h>
#include <misc/slist.h>
#include <net/net_core.h>
#include <net/nbuf.h>
#include <net/net_if.h>
#include <net/net_stats.h>
#include "net_private.h"
#include "icmpv6.h"

static sys_slist_t handlers;

void net_icmpv6_register_handler(struct net_icmpv6_handler *handler)
{
	sys_slist_prepend(&handlers, &handler->node);
}

static enum net_verdict handle_echo_request(struct net_buf *buf)
{
	/* Note that we send the same data buffers back and just swap
	 * the addresses etc.
	 */
#if NET_DEBUG > 0
	char out[sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx")];

	snprintf(out, sizeof(out),
		 net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->dst));
	NET_DBG("Received Echo Request from %s to %s",
		net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->src), out);
#endif /* NET_DEBUG > 0 */

	if (net_is_ipv6_addr_mcast(&NET_IPV6_BUF(buf)->dst)) {
		net_ipaddr_copy(&NET_IPV6_BUF(buf)->dst,
				&NET_IPV6_BUF(buf)->src);

		net_ipaddr_copy(&NET_IPV6_BUF(buf)->src,
				net_if_ipv6_select_src_addr(net_nbuf_iface(buf),
						&NET_IPV6_BUF(buf)->dst));
	} else {
		struct in6_addr addr;

		net_ipaddr_copy(&addr, &NET_IPV6_BUF(buf)->src);
		net_ipaddr_copy(&NET_IPV6_BUF(buf)->src,
				&NET_IPV6_BUF(buf)->dst);
		net_ipaddr_copy(&NET_IPV6_BUF(buf)->dst, &addr);
	}

	NET_IPV6_BUF(buf)->hop_limit =
		net_if_ipv6_get_hop_limit(net_nbuf_iface(buf));

	NET_ICMP_BUF(buf)->type = NET_ICMPV6_ECHO_REPLY;
	NET_ICMP_BUF(buf)->code = 0;
	NET_ICMP_BUF(buf)->chksum = 0;
	NET_ICMP_BUF(buf)->chksum = ~net_calc_chksum_icmpv6(buf);

	net_nbuf_ll_swap(buf);

#if NET_DEBUG > 0
	snprintf(out, sizeof(out),
		 net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->dst));
	NET_DBG("Sending Echo Reply from %s to %s",
		net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->src), out);
#endif /* NET_DEBUG > 0 */

	if (net_send_data(buf) < 0) {
		NET_STATS(++net_stats.icmp.drop);
		return NET_DROP;
	}

	NET_STATS(++net_stats.icmp.sent);

	return NET_OK;
}

static inline void setup_ipv6_header(struct net_buf *buf, uint8_t extra_len,
				     uint8_t hop_limit, uint8_t icmp_type,
				     uint8_t icmp_code)
{
	NET_IPV6_BUF(buf)->vtc = 0x60;
	NET_IPV6_BUF(buf)->tcflow = 0;
	NET_IPV6_BUF(buf)->flow = 0;
	NET_IPV6_BUF(buf)->len[0] = 0;
	NET_IPV6_BUF(buf)->len[1] = NET_ICMPH_LEN + extra_len +
		NET_ICMPV6_UNUSED_LEN;

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

int net_icmpv6_send_error(struct net_buf *orig, uint8_t type, uint8_t code)
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

	buf = net_nbuf_get_reserve_tx(0);

	/* We need to remember the original location of source and destination
	 * addresses as the net_nbuf_copy() will mangle the original buffer.
	 */
	src = &NET_IPV6_BUF(orig)->src;
	dst = &NET_IPV6_BUF(orig)->dst;

	/* There is unsed part in ICMPv6 error msg header */
	reserve = sizeof(struct net_ipv6_hdr) + sizeof(struct net_icmp_hdr) +
		NET_ICMPV6_UNUSED_LEN;

	if (NET_IPV6_BUF(orig)->nexthdr == IPPROTO_UDP) {
		extra_len = sizeof(struct net_ipv6_hdr) +
			sizeof(struct net_udp_hdr);
	} else if (NET_IPV6_BUF(orig)->nexthdr == IPPROTO_TCP) {
		extra_len = sizeof(struct net_ipv6_hdr);
		/* FIXME, add TCP header length too */
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
	frag = net_nbuf_copy(orig->frags, extra_len, reserve);
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

#if NET_DEBUG > 0
	do {
		char out[sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx")];
		snprintf(out, sizeof(out),
			 net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->dst));
		NET_DBG("Sending ICMPv6 Error Message type %d code %d "
			"from %s to %s", type, code,
			net_sprint_ipv6_addr(&NET_IPV6_BUF(buf)->src), out);
	} while (0);
#endif /* NET_DEBUG > 0 */

	if (net_send_data(buf) >= 0) {
		NET_STATS(++net_stats.icmp.sent);
		return -EIO;
	}

drop:
	net_nbuf_unref(buf);
	NET_STATS(++net_stats.icmp.drop);

	/* Note that we always return < 0 so that the caller knows to
	 * discard the original buffer.
	 */
	return -EIO;
}

enum net_verdict net_icmpv6_input(struct net_buf *buf, uint16_t len,
				  uint8_t type, uint8_t code)
{
	sys_snode_t *node;
	struct net_icmpv6_handler *cb;

	SYS_SLIST_FOR_EACH_NODE(&handlers, node) {
		cb = (struct net_icmpv6_handler *)node;

		if (!cb) {
			continue;
		}

		if (cb->type == type && (cb->code == code || cb->code == 0)) {
			NET_STATS(++net_stats.icmp.recv);
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
