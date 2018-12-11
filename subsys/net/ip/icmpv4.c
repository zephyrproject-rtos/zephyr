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

static int icmpv4_create(struct net_pkt *pkt, u8_t icmp_type, u8_t icmp_code)
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

int net_icmpv4_finalize(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmpv4_access,
					      struct net_icmp_hdr);
	struct net_icmp_hdr *icmp_hdr;

	icmp_hdr = (struct net_icmp_hdr *)net_pkt_get_data_new(pkt,
							       &icmpv4_access);
	if (!icmp_hdr) {
		return -ENOBUFS;
	}

	icmp_hdr->chksum = net_calc_chksum_icmpv4(pkt);

	return net_pkt_set_data(pkt, &icmpv4_access);
}

static enum net_verdict icmpv4_handle_echo_request(struct net_pkt *pkt,
						   struct net_ipv4_hdr *ip_hdr)
{
	struct net_pkt *reply = NULL;

	/* If interface can not select src address based on dst addr
	 * and src address is unspecified, drop the echo request.
	 */
	if (net_ipv4_is_addr_unspecified(&ip_hdr->src)) {
		NET_DBG("DROP: src addr is unspecified");
		goto drop;
	}

	NET_DBG("Received Echo Request from %s to %s",
		log_strdup(net_sprint_ipv4_addr(&ip_hdr->src)),
		log_strdup(net_sprint_ipv4_addr(&ip_hdr->dst)));

	net_pkt_cursor_init(pkt);

	/* Cloning is faster here as echo request might come with data behind */
	reply = net_pkt_clone_new(pkt, PKT_WAIT_TIME);
	if (!reply) {
		NET_DBG("DROP: No buffer");
		goto drop;
	}

	/* Let's keep the original data,
	 * we will only overwrite what is relevant
	 */
	net_pkt_set_overwrite(reply, true);

	if (net_ipv4_create_new(reply, &ip_hdr->dst, &ip_hdr->src) ||
	    icmpv4_create(reply, NET_ICMPV4_ECHO_REPLY, 0)) {
		NET_DBG("DROP: wrong buffer");
		goto drop;
	}

	net_pkt_cursor_init(reply);
	net_ipv4_finalize_new(reply, IPPROTO_ICMP);

	NET_DBG("Sending Echo Reply from %s to %s",
		log_strdup(net_sprint_ipv4_addr(&ip_hdr->dst)),
		log_strdup(net_sprint_ipv4_addr(&ip_hdr->src)));

	if (net_send_data(reply) < 0) {
		goto drop;
	}

	net_stats_update_icmp_sent(net_pkt_iface(reply));

	net_pkt_unref(pkt);

	return NET_OK;
drop:
	if (reply) {
		net_pkt_unref(reply);
	}

	net_stats_update_icmp_drop(net_pkt_iface(pkt));

	return NET_DROP;
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
	    icmpv4_create(pkt, NET_ICMPV4_ECHO_REQUEST, 0)) {
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

int net_icmpv4_send_error(struct net_pkt *orig, u8_t type, u8_t code)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	int err = -EIO;
	struct net_ipv4_hdr *ip_hdr;
	struct net_pkt *pkt;
	size_t copy_len;

	net_pkt_cursor_init(orig);

	ip_hdr = (struct net_ipv4_hdr *)net_pkt_get_data_new(orig,
							     &ipv4_access);
	if (!ip_hdr) {
		goto drop_no_pkt;
	}

	if (ip_hdr->proto == IPPROTO_ICMP) {
		NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmpv4_access,
						      struct net_icmp_hdr);
		struct net_icmp_hdr *icmp_hdr;

		icmp_hdr = (struct net_icmp_hdr *)net_pkt_get_data_new(
							orig, &icmpv4_access);
		if (!icmp_hdr || icmp_hdr->code < 8) {
			/* We must not send ICMP errors back */
			err = -EINVAL;
			goto drop_no_pkt;
		}
	}

	if (ip_hdr->proto == IPPROTO_UDP) {
		copy_len = sizeof(struct net_ipv4_hdr) +
			sizeof(struct net_udp_hdr);
	} else if (ip_hdr->proto == IPPROTO_TCP) {
		copy_len = sizeof(struct net_ipv4_hdr) +
			sizeof(struct net_tcp_hdr);
	} else {
		copy_len = 0;
	}

	pkt = net_pkt_alloc_with_buffer(net_pkt_iface(orig),
					copy_len + NET_ICMPV4_UNUSED_LEN,
					AF_INET, IPPROTO_ICMP,
					PKT_WAIT_TIME);
	if (!pkt) {
		err =  -ENOMEM;
		goto drop_no_pkt;
	}

	if (net_ipv4_create_new(pkt, &ip_hdr->dst, &ip_hdr->src) ||
	    icmpv4_create(pkt, type, code) ||
	    net_pkt_memset(pkt, 0, NET_ICMPV4_UNUSED_LEN) ||
	    net_pkt_copy_new(pkt, orig, copy_len)) {
		goto drop;
	}

	net_pkt_cursor_init(pkt);
	net_ipv4_finalize_new(pkt, IPPROTO_ICMP);

	net_pkt_lladdr_dst(pkt)->addr = net_pkt_lladdr_src(orig)->addr;
	net_pkt_lladdr_dst(pkt)->len = net_pkt_lladdr_src(orig)->len;

	NET_DBG("Sending ICMPv4 Error Message type %d code %d from %s to %s",
		type, code,
		log_strdup(net_sprint_ipv4_addr(&ip_hdr->src)),
		log_strdup(net_sprint_ipv4_addr(&ip_hdr->dst)));

	if (net_send_data(pkt) >= 0) {
		net_stats_update_icmp_sent(net_pkt_iface(orig));
		return 0;
	}

drop:
	net_pkt_unref(pkt);

drop_no_pkt:
	net_stats_update_icmp_drop(net_pkt_iface(orig));

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
				  struct net_ipv4_hdr *ip_hdr)
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

	if (net_ipv4_is_addr_bcast(net_pkt_iface(pkt), &ip_hdr->dst) &&
	    (!IS_ENABLED(CONFIG_NET_ICMPV4_ACCEPT_BROADCAST) ||
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
			return cb->handler(pkt, ip_hdr);
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
