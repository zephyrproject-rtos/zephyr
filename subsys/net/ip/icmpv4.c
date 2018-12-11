/** @file
 * @brief ICMPv4 related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_icmpv4, CONFIG_NET_ICMPV4_LOG_LEVEL);

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
	u16_t chksum = 0U;
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

	chksum = net_calc_chksum_icmpv4(pkt);

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
		log_strdup(net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src)),
		log_strdup(net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst)));

	net_ipaddr_copy(&addr, &NET_IPV4_HDR(pkt)->src);
	net_ipaddr_copy(&NET_IPV4_HDR(pkt)->src,
			net_if_ipv4_select_src_addr(net_pkt_iface(pkt),
						    &addr));
	/* If interface can not select src address based on dst addr
	 * and src address is unspecified, drop the echo request.
	 */
	if (net_ipv4_is_addr_unspecified(&NET_IPV4_HDR(pkt)->src)) {
		NET_DBG("DROP: src addr is unspecified");
		return NET_DROP;
	}

	net_ipaddr_copy(&NET_IPV4_HDR(pkt)->dst, &addr);

	icmp_hdr.type = NET_ICMPV4_ECHO_REPLY;
	icmp_hdr.code = 0;

	ret = net_icmpv4_set_hdr(pkt, &icmp_hdr);
	if (ret < 0) {
		return NET_DROP;
	}

	net_ipv4_finalize(pkt, IPPROTO_ICMP);

	NET_DBG("Sending Echo Reply from %s to %s",
		log_strdup(net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src)),
		log_strdup(net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst)));

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

static int icmpv4_create_new(struct net_pkt *pkt, u8_t icmp_type,
			     u8_t icmp_code)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmpv4_access,
					      struct net_icmp_hdr);
	struct net_icmp_hdr *icmp_hdr;

	icmp_hdr = (struct net_icmp_hdr *)net_pkt_get_data_new(pkt,
							       &icmpv4_access);
	if (!icmp_hdr) {
		return -ENOBUFS;
	}

	icmp_hdr->type   = icmp_type;
	icmp_hdr->code   = icmp_code;
	icmp_hdr->chksum = 0;

	return net_pkt_set_data(pkt, &icmpv4_access);
}

int net_icmpv4_send_echo_request(struct net_if *iface,
				 struct in_addr *dst,
				 u16_t identifier,
				 u16_t sequence)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmpv4_access,
					      struct net_icmpv4_echo_req);
	int ret = -ENOBUFS;
	struct net_icmpv4_echo_req *echo_req;
	const struct in_addr *src;
	struct net_pkt *pkt;

	if (!iface->config.ip.ipv4) {
		return -EINVAL;
	}

	/* Take the first address of the network interface */
	src = &iface->config.ip.ipv4->unicast[0].address.in_addr;

	pkt = net_pkt_alloc_with_buffer(iface,
					sizeof(struct net_icmpv4_echo_req),
					AF_INET, IPPROTO_ICMP,
					PKT_WAIT_TIME);
	if (!pkt) {
		return -ENOMEM;
	}

	if (net_ipv4_create_new(pkt, src, dst) ||
	    icmpv4_create_new(pkt, NET_ICMPV4_ECHO_REQUEST, 0)) {
		goto drop;
	}

	echo_req = (struct net_icmpv4_echo_req *)net_pkt_get_data_new(
							pkt, &icmpv4_access);
	if (!echo_req) {
		goto drop;
	}

	echo_req->identifier = htons(identifier);
	echo_req->sequence   = htons(sequence);

	net_pkt_set_data(pkt, &icmpv4_access);

	net_pkt_cursor_init(pkt);

	net_ipv4_finalize_new(pkt, IPPROTO_ICMP);

	NET_DBG("Sending ICMPv4 Echo Request type %d from %s to %s",
		NET_ICMPV4_ECHO_REQUEST,
		log_strdup(net_sprint_ipv4_addr(src)),
		log_strdup(net_sprint_ipv4_addr(dst)));

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

	pkt = net_pkt_get_reserve_tx(PKT_WAIT_TIME);
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
		log_strdup(net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->src)),
		log_strdup(net_sprint_ipv4_addr(&NET_IPV4_HDR(pkt)->dst)));

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

enum net_verdict net_icmpv4_input(struct net_pkt *pkt, bool bcast)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access,
					      struct net_icmp_hdr);
	struct net_icmp_hdr *icmp_hdr;
	struct net_icmpv4_handler *cb;

	icmp_hdr = (struct net_icmp_hdr *)net_pkt_get_data_new(pkt,
							       &icmp_access);
	if (!icmp_hdr) {
		NET_DBG("DROP: NULL ICMPv4 header");
		return NET_DROP;
	}

	if (net_calc_chksum_icmpv4(pkt) != 0) {
		NET_DBG("DROP: Invalid checksum");
		goto drop;
	}

	if (bcast && (!IS_ENABLED(CONFIG_NET_ICMPV4_ACCEPT_BROADCAST) ||
		      icmp_hdr->type != NET_ICMPV4_ECHO_REQUEST)) {
		NET_DBG("DROP: broadcast pkt");
		goto drop;
	}

	net_pkt_acknowledge_data(pkt, &icmp_access);

	NET_DBG("ICMPv4 packet received type %d code %d",
		icmp_hdr->type, icmp_hdr->code);

	net_stats_update_icmp_recv(net_pkt_iface(pkt));

	SYS_SLIST_FOR_EACH_CONTAINER(&handlers, cb, node) {
		if (cb->type == icmp_hdr->type &&
		    (cb->code == icmp_hdr->code || cb->code == 0)) {
			return cb->handler(pkt);
		}
	}

drop:
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
