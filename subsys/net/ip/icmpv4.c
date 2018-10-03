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
#include "ipv4.h"
#include "icmpv4.h"
#include "net_stats.h"

#define PKT_WAIT_TIME K_SECONDS(1)

static sys_slist_t handlers;

int net_icmpv4_set_hdr(struct net_pkt *pkt, struct net_icmp_hdr *hdr)
{
	struct net_buf *frag;
	u16_t pos;

	frag = net_pkt_write(pkt, pkt->frags,
				net_pkt_ip_hdr_len(pkt),
				&pos, sizeof(*hdr), (u8_t *)hdr,
				PKT_WAIT_TIME);
	if (pos > 0 && !frag) {
		return -EINVAL;
	}

	return 0;
}

int net_icmpv4_get_hdr(struct net_pkt *pkt, struct net_icmp_hdr *hdr)
{
	struct net_buf *frag;
	u16_t pos;

	frag = net_frag_read(pkt->frags, net_pkt_ip_hdr_len(pkt), &pos,
			     sizeof(*hdr), (u8_t *)hdr);
	if (pos > 0 && !frag) {
		return -EINVAL;
	}

	return 0;
}

int net_icmpv4_set_chksum(struct net_pkt *pkt)
{
	u16_t chksum = 0;
	struct net_buf *frag;
	struct net_buf *temp_frag;
	u16_t temp_pos;
	u16_t pos;

	frag = net_frag_skip(pkt->frags, 0, &pos,
			     net_pkt_ip_hdr_len(pkt) +
			     1 + 1 /* type + code */);
	if (pos > 0 && !frag) {
		return -EINVAL;
	}

	/* Cache checksum fragment and postion, to be safe side first
	 * write 0's in checksum position and calculate checksum and
	 * write checksum in the packet.
	 */
	temp_frag = frag;
	temp_pos = pos;

	frag = net_pkt_write(pkt, frag, pos, &pos, sizeof(chksum),
			     (u8_t *)&chksum, PKT_WAIT_TIME);
	if (pos > 0 && !frag) {
		return -EINVAL;
	}

	chksum = ~net_calc_chksum_icmpv4(pkt);

	temp_frag = net_pkt_write(pkt, temp_frag, temp_pos, &temp_pos,
				  sizeof(chksum), (u8_t *)&chksum,
				  PKT_WAIT_TIME);
	if (temp_pos > 0 && !temp_frag) {
		return -EINVAL;
	}

	return 0;
}

static inline enum net_verdict icmpv4_handle_echo_request(struct net_pkt *pkt)
{
	/* Note that we send the same data packets back and just swap
	 * the addresses etc.
	 */
	struct net_icmp_hdr icmp_hdr;
	struct in_addr addr;
	int ret;

	NET_DBG("Received Echo Request from %s to %s",
		net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src),
		net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst));

	net_ipaddr_copy(&addr, &NET_IPV4_HDR(pkt)->src);
	net_ipaddr_copy(&NET_IPV4_HDR(pkt)->src,
			&NET_IPV4_HDR(pkt)->dst);
	net_ipaddr_copy(&NET_IPV4_HDR(pkt)->dst, &addr);

	icmp_hdr.type = NET_ICMPV4_ECHO_REPLY;
	icmp_hdr.code = 0;

	ret = net_icmpv4_set_hdr(pkt, &icmp_hdr);
	if (ret < 0) {
		return NET_DROP;
	}

	ret = net_icmpv4_set_chksum(pkt);
	if (ret < 0) {
		return NET_DROP;
	}

	NET_DBG("Sending Echo Reply from %s to %s",
		net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src),
		net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst));

	if (net_send_data(pkt) < 0) {
		net_stats_update_icmp_drop(net_pkt_iface(pkt));
		return NET_DROP;
	}

	net_stats_update_icmp_sent(net_pkt_iface(pkt));

	return NET_OK;
}

static struct net_buf *icmpv4_create(struct net_pkt *pkt, u8_t icmp_type,
				     u8_t icmp_code)
{
	struct net_buf *frag = pkt->frags;
	u16_t pos;

	net_buf_add(frag, sizeof(struct net_icmp_hdr));

	frag = net_pkt_write_u8_timeout(pkt, frag, net_pkt_ip_hdr_len(pkt),
					&pos, icmp_type, PKT_WAIT_TIME);
	frag = net_pkt_write_u8_timeout(pkt, frag, pos, &pos, icmp_code,
					PKT_WAIT_TIME);
	return frag;
}

int net_icmpv4_send_echo_request(struct net_if *iface,
				 struct in_addr *dst,
				 u16_t identifier,
				 u16_t sequence)
{
	struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
	const struct in_addr *src;
	struct net_pkt *pkt;
	int ret;

	if (!ipv4) {
		return -EINVAL;
	}

	/* Take the first address of the network interface */
	src = &ipv4->unicast[0].address.in_addr;

	/* We cast to IPv6 address but that should be ok in this case
	 * as IPv4 cannot be used in 802.15.4 where it is the reserve
	 * size can change depending on address.
	 */
	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface,
					      (const struct in6_addr *)dst),
				     PKT_WAIT_TIME);
	if (!pkt) {
		return -ENOMEM;
	}

	net_pkt_set_iface(pkt, iface);

	if (!net_ipv4_create(pkt, src, dst, iface, IPPROTO_ICMP)) {
		ret = -ENOMEM;
		goto drop;
	}

	if (!icmpv4_create(pkt, NET_ICMPV4_ECHO_REQUEST, 0)) {
		ret = -ENOMEM;
		goto drop;
	}

	net_buf_add(pkt->frags, sizeof(struct net_icmpv4_echo_req));

	NET_ICMPV4_ECHO_REQ(pkt)->identifier = htons(identifier);
	NET_ICMPV4_ECHO_REQ(pkt)->sequence = htons(sequence);

	net_ipv4_finalize(pkt, IPPROTO_ICMP);

	NET_DBG("Sending ICMPv4 Echo Request type %d from %s to %s",
		NET_ICMPV4_ECHO_REQUEST,
		net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src),
		net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst));

	if (net_send_data(pkt) >= 0) {
		net_stats_update_icmp_sent(iface);
		return 0;
	}

	net_stats_update_icmp_drop(iface);

	ret = -EIO;

drop:
	net_pkt_unref(pkt);

	return ret;
}

#define append(pkt, type, value)					\
	do {								\
		if (!net_pkt_append_##type##_timeout(pkt, value,	\
						     PKT_WAIT_TIME)) {	\
			err = -ENOMEM;					\
			goto drop;					\
		}							\
	} while (0)

int net_icmpv4_send_error(struct net_pkt *orig, u8_t type, u8_t code)
{
	int err = -EIO;
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_if *iface;
	size_t copy_len;
	const struct in_addr *src, *dst;

	iface = net_pkt_iface(orig);

	if (NET_IPV4_HDR(orig)->proto == IPPROTO_ICMP) {
		struct net_icmp_hdr icmp_hdr;

		err = net_icmpv4_get_hdr(orig, &icmp_hdr);
		if (err < 0 || icmp_hdr.code < 8) {
			/* We must not send ICMP errors back */
			goto drop_no_pkt;
		}
	}

	dst = &NET_IPV4_HDR(orig)->src;
	src = &NET_IPV4_HDR(orig)->dst;

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(iface,
					      (const struct in6_addr *)dst),
				     PKT_WAIT_TIME);
	if (!pkt) {
		err = -ENOMEM;
		goto drop_no_pkt;
	}

	net_pkt_set_iface(pkt, iface);

	if (!net_ipv4_create(pkt, src, dst, iface, IPPROTO_ICMP)) {
		err = -ENOMEM;
		goto drop;
	}

	if (!icmpv4_create(pkt, type, code)) {
		err = -ENOMEM;
		goto drop;
	}

	/* Appending unused part, filled with 0s */
	append(pkt, be32, 0);

	if (NET_IPV4_HDR(orig)->proto == IPPROTO_UDP) {
		copy_len = sizeof(struct net_ipv4_hdr) +
			sizeof(struct net_udp_hdr);
	} else if (NET_IPV4_HDR(orig)->proto == IPPROTO_TCP) {
		copy_len = sizeof(struct net_ipv4_hdr);
		/* FIXME, add TCP header length too */
	} else {
		copy_len = 0;
	}

	frag = net_pkt_copy(orig, copy_len, 0, PKT_WAIT_TIME);
	if (!frag) {
		err = -ENOMEM;
		goto drop;
	}

	net_pkt_frag_add(pkt, frag);

	net_ipv4_finalize(pkt, IPPROTO_ICMP);

	net_pkt_lladdr_dst(pkt)->addr = net_pkt_lladdr_src(orig)->addr;
	net_pkt_lladdr_dst(pkt)->len = net_pkt_lladdr_src(orig)->len;

	NET_DBG("Sending ICMPv4 Error Message type %d code %d from %s to %s",
		type, code,
		net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src),
		net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst));

	if (net_send_data(pkt) >= 0) {
		net_stats_update_icmp_sent(iface);
		return 0;
	}

drop:
	net_pkt_unref(pkt);

drop_no_pkt:
	net_stats_update_icmp_drop(iface);

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

enum net_verdict net_icmpv4_input(struct net_pkt *pkt)
{
	struct net_icmpv4_handler *cb;
	struct net_icmp_hdr icmp_hdr;
	int ret;

	ret = net_icmpv4_get_hdr(pkt, &icmp_hdr);
	if (ret < 0) {
		NET_DBG("NULL ICMPv4 header - dropping");
		return NET_DROP;
	}

	NET_DBG("ICMPv4 packet received type %d code %d",
		icmp_hdr.type, icmp_hdr.code);

	net_stats_update_icmp_recv(net_pkt_iface(pkt));

	SYS_SLIST_FOR_EACH_CONTAINER(&handlers, cb, node) {
		if (cb->type == icmp_hdr.type &&
				(cb->code == icmp_hdr.code || cb->code == 0)) {
			return cb->handler(pkt);
		}
	}

	net_stats_update_icmp_drop(net_pkt_iface(pkt));

	return NET_DROP;
}

static struct net_icmpv4_handler echo_request_handler = {
	.type = NET_ICMPV4_ECHO_REQUEST,
	.code = 0,
	.handler = icmpv4_handle_echo_request,
};

void net_icmpv4_init(void)
{
	net_icmpv4_register_handler(&echo_request_handler);
}
