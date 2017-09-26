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
#include <net/net_pkt.h>
#include <net/net_if.h>
#include "net_private.h"
#include "icmpv4.h"
#include "net_stats.h"

#define PKT_WAIT_TIME K_SECONDS(1)

static sys_slist_t handlers;

struct net_icmp_hdr *net_icmpv4_set_hdr(struct net_pkt *pkt,
					struct net_icmp_hdr *hdr)
{
	struct net_icmp_hdr *icmp_hdr;
	struct net_buf *frag;
	u16_t pos;

	icmp_hdr = net_pkt_icmp_data(pkt);
	if (net_icmp_header_fits(pkt, icmp_hdr)) {
		return icmp_hdr;
	}

	frag = net_pkt_write_u8(pkt, pkt->frags,
				net_pkt_ip_hdr_len(pkt),
				&pos, hdr->type);
	frag = net_pkt_write_u8(pkt, frag, pos, &pos, hdr->code);
	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(hdr->chksum),
			     (u8_t *)&hdr->chksum, PKT_WAIT_TIME);
	if (!frag) {
		NET_ASSERT(frag);
		return NULL;
	}

	return hdr;
}

struct net_icmp_hdr *net_icmpv4_get_hdr(struct net_pkt *pkt,
					struct net_icmp_hdr *hdr)
{
	struct net_icmp_hdr *icmp_hdr;
	struct net_buf *frag;
	u16_t pos;

	icmp_hdr = net_pkt_icmp_data(pkt);
	if (net_icmp_header_fits(pkt, icmp_hdr)) {
		return icmp_hdr;
	}

	frag = net_frag_read_u8(pkt->frags, net_pkt_ip_hdr_len(pkt), &pos,
				&hdr->type);
	frag = net_frag_read_u8(frag, pos, &pos, &hdr->code);
	frag = net_frag_read(frag, pos, &pos, sizeof(hdr->chksum),
			     (u8_t *)&hdr->chksum);
	if (!frag) {
		NET_ASSERT(frag);
		return NULL;
	}

	return hdr;
}

struct net_buf *net_icmpv4_set_chksum(struct net_pkt *pkt,
				      struct net_buf *frag)
{
	struct net_icmp_hdr *icmp_hdr;
	u16_t chksum = 0;
	u16_t pos;

	icmp_hdr = net_pkt_icmp_data(pkt);
	if (net_icmp_header_fits(pkt, icmp_hdr)) {
		icmp_hdr->chksum = 0;
		icmp_hdr->chksum = ~net_calc_chksum_icmpv4(pkt);

		return frag;
	}

	frag = net_pkt_write(pkt, frag,
			     net_pkt_ip_hdr_len(pkt) +
			     1 + 1 /* type + code */, &pos,
			     sizeof(chksum), (u8_t *)&chksum, PKT_WAIT_TIME);

	chksum = ~net_calc_chksum_icmpv4(pkt);

	frag = net_pkt_write(pkt, frag, pos - 2, &pos, sizeof(chksum),
			     (u8_t *)&chksum, PKT_WAIT_TIME);

	NET_ASSERT(frag);

	return frag;
}

static inline enum net_verdict handle_echo_request(struct net_pkt *pkt)
{
	/* Note that we send the same data packets back and just swap
	 * the addresses etc.
	 */
	struct net_icmp_hdr hdr, *icmp_hdr;
	struct in_addr addr;

#if defined(CONFIG_NET_DEBUG_ICMPV4)
	char out[sizeof("xxx.xxx.xxx.xxx")];

	snprintk(out, sizeof(out), "%s",
		 net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst));
	NET_DBG("Received Echo Request from %s to %s",
		net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src), out);
#endif /* CONFIG_NET_DEBUG_ICMPV4 */

	net_ipaddr_copy(&addr, &NET_IPV4_HDR(pkt)->src);
	net_ipaddr_copy(&NET_IPV4_HDR(pkt)->src,
			&NET_IPV4_HDR(pkt)->dst);
	net_ipaddr_copy(&NET_IPV4_HDR(pkt)->dst, &addr);

	icmp_hdr = net_icmpv4_get_hdr(pkt, &hdr);
	icmp_hdr->type = NET_ICMPV4_ECHO_REPLY;
	icmp_hdr->code = 0;
	icmp_hdr->chksum = 0;
	net_icmpv4_set_hdr(pkt, icmp_hdr);
	net_icmpv4_set_chksum(pkt, pkt->frags);

#if defined(CONFIG_NET_DEBUG_ICMPV4)
	snprintk(out, sizeof(out), "%s",
		 net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst));
	NET_DBG("Sending Echo Reply from %s to %s",
		net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src), out);
#endif /* CONFIG_NET_DEBUG_ICMPV4 */

	if (net_send_data(pkt) < 0) {
		net_stats_update_icmp_drop();
		return NET_DROP;
	}

	net_stats_update_icmp_sent();

	return NET_OK;
}

#define NET_ICMPV4_UNUSED_LEN 4

static inline void setup_ipv4_header(struct net_pkt *pkt, u8_t extra_len,
				     u8_t ttl, u8_t icmp_type,
				     u8_t icmp_code)
{
	struct net_buf *frag = pkt->frags;
	u16_t pos;

	NET_IPV4_HDR(pkt)->vhl = 0x45;
	NET_IPV4_HDR(pkt)->tos = 0x00;
	NET_IPV4_HDR(pkt)->len[0] = 0;
	NET_IPV4_HDR(pkt)->len[1] = sizeof(struct net_ipv4_hdr) +
		NET_ICMPH_LEN + extra_len + NET_ICMPV4_UNUSED_LEN;

	NET_IPV4_HDR(pkt)->proto = IPPROTO_ICMP;
	NET_IPV4_HDR(pkt)->ttl = ttl;
	NET_IPV4_HDR(pkt)->offset[0] = NET_IPV4_HDR(pkt)->offset[1] = 0;
	NET_IPV4_HDR(pkt)->id[0] = NET_IPV4_HDR(pkt)->id[1] = 0;

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	NET_IPV4_HDR(pkt)->chksum = 0;
	NET_IPV4_HDR(pkt)->chksum = ~net_calc_chksum_ipv4(pkt);

	frag = net_pkt_write_u8(pkt, frag, net_pkt_ip_hdr_len(pkt), &pos,
				icmp_type);
	frag = net_pkt_write_u8(pkt, frag, pos, &pos, icmp_code);
	net_pkt_write_be32(pkt, frag, pos, &pos, 0);
}

int net_icmpv4_send_echo_request(struct net_if *iface,
				 struct in_addr *dst,
				 u16_t identifier,
				 u16_t sequence)
{
	const struct in_addr *src;
	struct net_pkt *pkt;
	struct net_buf *frag;

	/* Take the first address of the network interface */
	src = &iface->ipv4.unicast[0].address.in_addr;

	/* We cast to IPv6 address but that should be ok in this case
	 * as IPv4 cannot be used in 802.15.4 where it is the reserve
	 * size can change depending on address.
	 */
	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface,
					      (const struct in6_addr *)dst),
				     K_FOREVER);

	frag = net_pkt_get_frag(pkt, K_FOREVER);

	net_pkt_frag_add(pkt, frag);
	net_pkt_set_family(pkt, AF_INET);
	net_pkt_set_iface(pkt, iface);

	net_buf_add(pkt->frags, sizeof(struct net_ipv4_hdr) +
		    sizeof(struct net_icmp_hdr) +
		    sizeof(struct net_icmpv4_echo_req));

	setup_ipv4_header(pkt, 0, net_if_ipv4_get_ttl(iface),
			  NET_ICMPV4_ECHO_REQUEST, 0);

	net_ipaddr_copy(&NET_IPV4_HDR(pkt)->src, src);
	net_ipaddr_copy(&NET_IPV4_HDR(pkt)->dst, dst);

	NET_ICMPV4_ECHO_REQ(pkt)->identifier = htons(identifier);
	NET_ICMPV4_ECHO_REQ(pkt)->sequence = htons(sequence);

#if defined(CONFIG_NET_DEBUG_ICMPV4)
	do {
		char out[NET_IPV4_ADDR_LEN];

		snprintk(out, sizeof(out), "%s",
			 net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst));

		NET_DBG("Sending ICMPv4 Echo Request type %d"
			" from %s to %s", NET_ICMPV4_ECHO_REQUEST,
			net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src), out);
	} while (0);
#endif /* CONFIG_NET_DEBUG_ICMPV4 */

	net_icmpv4_set_chksum(pkt, pkt->frags);

	if (net_send_data(pkt) >= 0) {
		net_stats_update_icmp_sent();
		return 0;
	}

	net_pkt_unref(pkt);
	net_stats_update_icmp_drop();

	return -EIO;
}

int net_icmpv4_send_error(struct net_pkt *orig, u8_t type, u8_t code)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_if *iface = net_pkt_iface(orig);
	size_t extra_len, reserve;
	struct in_addr addr, *src, *dst;
	int err = -EIO;

	if (NET_IPV4_HDR(orig)->proto == IPPROTO_ICMP) {
		struct net_icmp_hdr icmp_hdr[1];

		if (!net_icmpv4_get_hdr(orig, icmp_hdr) ||
		    icmp_hdr->code < 8) {
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

	reserve = sizeof(struct net_ipv4_hdr) + sizeof(struct net_icmp_hdr) +
		NET_ICMPV4_UNUSED_LEN;

	if (NET_IPV4_HDR(orig)->proto == IPPROTO_UDP) {
		extra_len = sizeof(struct net_ipv4_hdr) +
			sizeof(struct net_udp_hdr);
	} else if (NET_IPV4_HDR(orig)->proto == IPPROTO_TCP) {
		extra_len = sizeof(struct net_ipv4_hdr);
		/* FIXME, add TCP header length too */
	} else {
		size_t space = CONFIG_NET_BUF_DATA_SIZE -
			net_if_get_ll_reserve(iface, NULL);

		if (reserve > space) {
			extra_len = 0;
		} else {
			extra_len = space - reserve;
		}
	}

	/* We need to remember the original location of source and destination
	 * addresses as the net_pkt_copy() will mangle the original packet.
	 */
	src = &NET_IPV4_HDR(orig)->src;
	dst = &NET_IPV4_HDR(orig)->dst;

	/* We only copy minimal IPv4 + next header from original message.
	 * This is so that the memory pressure is minimized.
	 */
	frag = net_pkt_copy(orig, extra_len, reserve, PKT_WAIT_TIME);
	if (!frag) {
		err = -ENOMEM;
		goto drop;
	}

	net_pkt_frag_add(pkt, frag);
	net_pkt_set_family(pkt, AF_INET);
	net_pkt_set_iface(pkt, iface);
	net_pkt_set_ll_reserve(pkt, net_buf_headroom(frag));

	setup_ipv4_header(pkt, extra_len, net_if_ipv4_get_ttl(iface),
			  type, code);

	net_ipaddr_copy(&addr, src);
	net_ipaddr_copy(&NET_IPV4_HDR(pkt)->src, dst);
	net_ipaddr_copy(&NET_IPV4_HDR(pkt)->dst, &addr);

	net_pkt_ll_src(pkt)->addr = net_pkt_ll_dst(orig)->addr;
	net_pkt_ll_src(pkt)->len = net_pkt_ll_dst(orig)->len;
	net_pkt_ll_dst(pkt)->addr = net_pkt_ll_src(orig)->addr;
	net_pkt_ll_dst(pkt)->len = net_pkt_ll_src(orig)->len;

	net_icmpv4_set_chksum(pkt, pkt->frags);

#if defined(CONFIG_NET_DEBUG_ICMPV4)
	do {
		char out[sizeof("xxx.xxx.xxx.xxx")];

		snprintk(out, sizeof(out), "%s",
			 net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst));
		NET_DBG("Sending ICMPv4 Error Message type %d code %d "
			"from %s to %s", type, code,
			net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src), out);
	} while (0);
#endif /* CONFIG_NET_DEBUG_ICMPV4 */

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

void net_icmpv4_register_handler(struct net_icmpv4_handler *handler)
{
	sys_slist_prepend(&handlers, &handler->node);
}

void net_icmpv4_unregister_handler(struct net_icmpv4_handler *handler)
{
	sys_slist_find_and_remove(&handlers, &handler->node);
}

enum net_verdict net_icmpv4_input(struct net_pkt *pkt,
				  u8_t type, u8_t code)
{
	struct net_icmpv4_handler *cb;

	net_stats_update_icmp_recv();

	SYS_SLIST_FOR_EACH_CONTAINER(&handlers, cb, node) {
		if (cb->type == type && (cb->code == code || cb->code == 0)) {
			return cb->handler(pkt);
		}
	}

	net_stats_update_icmp_drop();

	return NET_DROP;
}

static struct net_icmpv4_handler echo_request_handler = {
	.type = NET_ICMPV4_ECHO_REQUEST,
	.code = 0,
	.handler = handle_echo_request,
};

void net_icmpv4_init(void)
{
	net_icmpv4_register_handler(&echo_request_handler);
}
