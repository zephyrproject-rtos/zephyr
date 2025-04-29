/** @file
 * @brief ICMPv6 related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_icmpv6, CONFIG_NET_ICMPV6_LOG_LEVEL);

#include <errno.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/icmp.h>
#include "net_private.h"
#include "icmpv6.h"
#include "ipv6.h"
#include "net_stats.h"

#define PKT_WAIT_TIME K_SECONDS(1)

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

int net_icmpv6_finalize(struct net_pkt *pkt, bool force_chksum)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access,
					      struct net_icmp_hdr);
	struct net_icmp_hdr *icmp_hdr;

	icmp_hdr = (struct net_icmp_hdr *)net_pkt_get_data(pkt, &icmp_access);
	if (!icmp_hdr) {
		return -ENOBUFS;
	}

	icmp_hdr->chksum = 0U;
	if (net_if_need_calc_tx_checksum(net_pkt_iface(pkt), NET_IF_CHECKSUM_IPV6_ICMP) ||
		force_chksum) {
		icmp_hdr->chksum = net_calc_chksum_icmpv6(pkt);
		net_pkt_set_chksum_done(pkt, true);
	}

	return net_pkt_set_data(pkt, &icmp_access);
}

int net_icmpv6_create(struct net_pkt *pkt, uint8_t icmp_type, uint8_t icmp_code)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access,
					      struct net_icmp_hdr);
	struct net_icmp_hdr *icmp_hdr;

	icmp_hdr = (struct net_icmp_hdr *)net_pkt_get_data(pkt, &icmp_access);
	if (!icmp_hdr) {
		return -ENOBUFS;
	}

	icmp_hdr->type   = icmp_type;
	icmp_hdr->code   = icmp_code;
	icmp_hdr->chksum = 0U;

	return net_pkt_set_data(pkt, &icmp_access);
}

static int icmpv6_handle_echo_request(struct net_icmp_ctx *ctx,
				      struct net_pkt *pkt,
				      struct net_icmp_ip_hdr *hdr,
				      struct net_icmp_hdr *icmp_hdr,
				      void *user_data)
{
	struct net_pkt *reply = NULL;
	struct net_ipv6_hdr *ip_hdr = hdr->ipv6;
	const struct in6_addr *src;
	int16_t payload_len;

	ARG_UNUSED(user_data);
	ARG_UNUSED(icmp_hdr);

	NET_DBG("Received Echo Request from %s to %s",
		net_sprint_ipv6_addr(&ip_hdr->src),
		net_sprint_ipv6_addr(&ip_hdr->dst));

	payload_len = ntohs(ip_hdr->len) -
		net_pkt_ipv6_ext_len(pkt) - NET_ICMPH_LEN;
	if (payload_len < NET_ICMPV6_UNUSED_LEN) {
		/* No identifier or sequence number present */
		goto drop;
	}

	reply = net_pkt_alloc_with_buffer(net_pkt_iface(pkt), payload_len,
					  AF_INET6, IPPROTO_ICMPV6,
					  PKT_WAIT_TIME);
	if (!reply) {
		NET_DBG("DROP: No buffer");
		goto drop;
	}

	if (net_ipv6_is_addr_mcast((struct in6_addr *)ip_hdr->dst)) {
		src = net_if_ipv6_select_src_addr(net_pkt_iface(pkt),
						  (struct in6_addr *)ip_hdr->src);

		if (net_ipv6_is_addr_unspecified(src)) {
			NET_DBG("DROP: No src address match");
			goto drop;
		}
	} else {
		src = (struct in6_addr *)ip_hdr->dst;
	}

	/* We must not set the destination ll address here but trust
	 * that it is set properly using a value from neighbor cache.
	 * Same for source as it points to original pkt ll src address.
	 */
	(void)net_linkaddr_clear(net_pkt_lladdr_dst(reply));
	(void)net_linkaddr_clear(net_pkt_lladdr_src(reply));

	net_pkt_set_ip_dscp(reply, net_pkt_ip_dscp(pkt));
	net_pkt_set_ip_ecn(reply, net_pkt_ip_ecn(pkt));

	if (net_ipv6_create(reply, src, (struct in6_addr *)ip_hdr->src)) {
		NET_DBG("DROP: wrong buffer");
		goto drop;
	}

	if (net_icmpv6_create(reply, NET_ICMPV6_ECHO_REPLY, 0) ||
	    net_pkt_copy(reply, pkt, payload_len)) {
		NET_DBG("DROP: wrong buffer");
		goto drop;
	}

	net_pkt_cursor_init(reply);
	net_ipv6_finalize(reply, IPPROTO_ICMPV6);

	NET_DBG("Sending Echo Reply from %s to %s",
		net_sprint_ipv6_addr(src),
		net_sprint_ipv6_addr(&ip_hdr->src));

	if (net_try_send_data(reply, K_NO_WAIT) < 0) {
		goto drop;
	}

	net_stats_update_icmp_sent(net_pkt_iface(reply));

	return 0;

drop:
	if (reply) {
		net_pkt_unref(reply);
	}

	net_stats_update_icmp_drop(net_pkt_iface(pkt));

	return -EIO;
}

int net_icmpv6_send_error(struct net_pkt *orig, uint8_t type, uint8_t code,
			  uint32_t param)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	int err = -EIO;
	struct net_ipv6_hdr *ip_hdr;
	const struct in6_addr *src;
	struct net_pkt *pkt;
	size_t copy_len;
	int ret;

	net_pkt_cursor_init(orig);

	ip_hdr = (struct net_ipv6_hdr *)net_pkt_get_data(orig, &ipv6_access);
	if (!ip_hdr) {
		goto drop_no_pkt;
	}

	if (ip_hdr->nexthdr == IPPROTO_ICMPV6) {
		NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmpv6_access,
						      struct net_icmp_hdr);
		struct net_icmp_hdr *icmp_hdr;

		net_pkt_acknowledge_data(orig, &ipv6_access);

		icmp_hdr = (struct net_icmp_hdr *)net_pkt_get_data(
							orig, &icmpv6_access);
		if (!icmp_hdr || icmp_hdr->type < 128) {
			/* We must not send ICMP errors back */
			err = -EINVAL;
			goto drop_no_pkt;
		}

		net_pkt_cursor_init(orig);
	}

	if (ip_hdr->nexthdr == IPPROTO_UDP) {
		copy_len = sizeof(struct net_ipv6_hdr) +
			sizeof(struct net_udp_hdr);
	} else if (ip_hdr->nexthdr == IPPROTO_TCP) {
		copy_len = sizeof(struct net_ipv6_hdr) +
			sizeof(struct net_tcp_hdr);
	} else {
		copy_len = net_pkt_get_len(orig);
	}

	pkt = net_pkt_alloc_with_buffer(net_pkt_iface(orig),
					net_pkt_lladdr_src(orig)->len * 2 +
					copy_len + NET_ICMPV6_UNUSED_LEN,
					AF_INET6, IPPROTO_ICMPV6,
					PKT_WAIT_TIME);
	if (!pkt) {
		err = -ENOMEM;
		goto drop_no_pkt;
	}

	/* We created above a new packet that contains some extra space that we
	 * will use to store the destination and source link addresses. This is
	 * needed because we cannot use the original pkt, which contains the
	 * link address where the new packet will be sent, as that pkt might
	 * get re-used before we have managed to set the link addresses in L2
	 * as that (link address setting) happens in a different thread (TX)
	 * than this one.
	 * So we copy the destination and source link addresses here, then set
	 * the link address pointers correctly, and skip the needed space
	 * as the link address will be set in the pkt when the packet is
	 * constructed in L2. So basically all this for just to create some
	 * extra space for link addresses so that we can set the lladdr
	 * pointers in net_pkt.
	 */
	ret = net_pkt_write(pkt, net_pkt_lladdr_src(orig)->addr,
			    net_pkt_lladdr_src(orig)->len);
	if (ret < 0) {
		err = ret;
		goto drop;
	}

	memcpy(net_pkt_lladdr_dst(pkt)->addr, pkt->buffer->data,
	       net_pkt_lladdr_dst(orig)->len);

	ret = net_pkt_write(pkt, net_pkt_lladdr_dst(orig)->addr,
			    net_pkt_lladdr_dst(orig)->len);
	if (ret < 0) {
		err = ret;
		goto drop;
	}

	net_buf_pull_mem(pkt->buffer, net_pkt_lladdr_dst(orig)->len);

	memcpy(net_pkt_lladdr_src(pkt)->addr, pkt->buffer->data,
	       net_pkt_lladdr_src(orig)->len);

	net_buf_pull_mem(pkt->buffer, net_pkt_lladdr_src(orig)->len);

	net_pkt_lladdr_src(pkt)->len = net_pkt_lladdr_dst(orig)->len;
	net_pkt_lladdr_dst(pkt)->len = net_pkt_lladdr_src(orig)->len;

	if (net_ipv6_is_addr_mcast((struct in6_addr *)ip_hdr->dst)) {
		src = net_if_ipv6_select_src_addr(net_pkt_iface(pkt),
						  (struct in6_addr *)ip_hdr->dst);
	} else {
		src = (struct in6_addr *)ip_hdr->dst;
	}

	if (net_ipv6_create(pkt, src, (struct in6_addr *)ip_hdr->src) ||
	    net_icmpv6_create(pkt, type, code)) {
		goto drop;
	}

	/* Depending on error option, we store the param into the ICMP message.
	 */
	if (type == NET_ICMPV6_PARAM_PROBLEM) {
		err = net_pkt_write_be32(pkt, param);
	} else {
		err = net_pkt_memset(pkt, 0, NET_ICMPV6_UNUSED_LEN);
	}

	/* Allocator might not have been able to allocate all requested space,
	 * so let's copy as much as we can.
	 */
	copy_len = net_pkt_available_buffer(pkt);

	if (err || net_pkt_copy(pkt, orig, copy_len)) {
		goto drop;
	}

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, IPPROTO_ICMPV6);

	NET_DBG("Sending ICMPv6 Error Message type %d code %d param %d"
		" from %s to %s", type, code, param,
		net_sprint_ipv6_addr(src),
		net_sprint_ipv6_addr(&ip_hdr->src));

	if (net_try_send_data(pkt, K_NO_WAIT) >= 0) {
		net_stats_update_icmp_sent(net_pkt_iface(pkt));
		return 0;
	}

drop:
	net_pkt_unref(pkt);

drop_no_pkt:
	net_stats_update_icmp_drop(net_pkt_iface(orig));

	return err;
}

enum net_verdict net_icmpv6_input(struct net_pkt *pkt,
				  struct net_ipv6_hdr *ip_hdr)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access,
					      struct net_icmp_hdr);
	struct net_icmp_hdr *icmp_hdr;
	int ret;

	icmp_hdr = (struct net_icmp_hdr *)net_pkt_get_data(pkt, &icmp_access);
	if (!icmp_hdr) {
		NET_DBG("DROP: NULL ICMPv6 header");
		return NET_DROP;
	}


	if (net_if_need_calc_rx_checksum(net_pkt_iface(pkt), NET_IF_CHECKSUM_IPV6_ICMP) ||
	    net_pkt_is_ip_reassembled(pkt)) {
		if (net_calc_chksum_icmpv6(pkt) != 0U) {
			NET_DBG("DROP: invalid checksum");
			goto drop;
		}
	}

	net_pkt_acknowledge_data(pkt, &icmp_access);

	NET_DBG("ICMPv6 %s received type %d code %d",
		net_icmpv6_type2str(icmp_hdr->type),
		icmp_hdr->type, icmp_hdr->code);

	net_stats_update_icmp_recv(net_pkt_iface(pkt));

	ret = net_icmp_call_ipv6_handlers(pkt, ip_hdr, icmp_hdr);
	if (ret < 0 && ret != -ENOENT) {
		NET_ERR("ICMPv6 handling failure (%d)", ret);
	}

	net_pkt_unref(pkt);

	return NET_OK;

drop:
	net_stats_update_icmp_drop(net_pkt_iface(pkt));

	return NET_DROP;
}

void net_icmpv6_init(void)
{
	static struct net_icmp_ctx ctx;
	int ret;

	ret = net_icmp_init_ctx(&ctx, NET_ICMPV6_ECHO_REQUEST, 0, icmpv6_handle_echo_request);
	if (ret < 0) {
		NET_ERR("Cannot register %s handler (%d)", STRINGIFY(NET_ICMPV6_ECHO_REQUEST),
			ret);
	}
}
