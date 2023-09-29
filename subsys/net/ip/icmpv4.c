/** @file
 * @brief ICMPv4 related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_icmpv4, CONFIG_NET_ICMPV4_LOG_LEVEL);

#include <errno.h>
#include <zephyr/sys/slist.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/icmp.h>
#include "net_private.h"
#include "ipv4.h"
#include "icmpv4.h"
#include "net_stats.h"

#define PKT_WAIT_TIME K_SECONDS(1)

struct net_icmpv4_hdr_opts_data {
	struct net_pkt *reply;
	const struct in_addr *src;
};

int net_icmpv4_create(struct net_pkt *pkt, uint8_t icmp_type, uint8_t icmp_code)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmpv4_access,
					      struct net_icmp_hdr);
	struct net_icmp_hdr *icmp_hdr;

	icmp_hdr = (struct net_icmp_hdr *)net_pkt_get_data(pkt, &icmpv4_access);
	if (!icmp_hdr) {
		return -ENOBUFS;
	}

	icmp_hdr->type   = icmp_type;
	icmp_hdr->code   = icmp_code;
	icmp_hdr->chksum = 0U;

	return net_pkt_set_data(pkt, &icmpv4_access);
}

int net_icmpv4_finalize(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmpv4_access,
					      struct net_icmp_hdr);
	struct net_icmp_hdr *icmp_hdr;

	if (IS_ENABLED(CONFIG_NET_IPV4_HDR_OPTIONS)) {
		if (net_pkt_skip(pkt, net_pkt_ipv4_opts_len(pkt))) {
			return -ENOBUFS;
		}
	}

	icmp_hdr = (struct net_icmp_hdr *)net_pkt_get_data(pkt, &icmpv4_access);
	if (!icmp_hdr) {
		return -ENOBUFS;
	}

	icmp_hdr->chksum = 0U;
	if (net_if_need_calc_tx_checksum(net_pkt_iface(pkt))) {
		icmp_hdr->chksum = net_calc_chksum_icmpv4(pkt);
	}

	return net_pkt_set_data(pkt, &icmpv4_access);
}

#if defined(CONFIG_NET_IPV4_HDR_OPTIONS)

/* Parse Record Route and add our own IP address based on
 * free entries.
 */
static int icmpv4_update_record_route(uint8_t *opt_data,
				      uint8_t opt_len,
				      struct net_pkt *reply,
				      const struct in_addr *src)
{
	uint8_t len = net_pkt_ipv4_opts_len(reply);
	uint8_t addr_len = sizeof(struct in_addr);
	uint8_t ptr_offset = 4U;
	uint8_t offset = 0U;
	uint8_t skip;
	uint8_t ptr;

	if (net_pkt_write_u8(reply, NET_IPV4_OPTS_RR)) {
		goto drop;
	}

	len++;

	if (net_pkt_write_u8(reply, opt_len + 2U)) {
		goto drop;
	}

	len++;

	/* The third octet is the pointer into the route data
	 * indicating the octet which begins the next area to
	 * store a route address. The pointer is relative to
	 * this option, and the smallest legal value for the
	 * pointer is 4.
	 */
	ptr = opt_data[offset++];

	/* If the route data area is already full (the pointer exceeds
	 * the length) the datagram is forwarded without inserting the
	 * address into the recorded route.
	 */
	if (ptr >= opt_len) {
		/* No free entry to update RecordRoute */
		if (net_pkt_write_u8(reply, ptr)) {
			goto drop;
		}

		len++;

		if (net_pkt_write(reply, opt_data + offset, opt_len)) {
			goto drop;
		}

		len += opt_len;

		net_pkt_set_ipv4_opts_len(reply, len);

		return 0;
	}

	/* If there is some room but not enough room for a full address
	 * to be inserted, the original datagram is considered to be in
	 * error and is discarded.
	 */
	if ((ptr + addr_len) > opt_len) {
		goto drop;
	}

	/* So, there is a free entry to update Record Route */
	if (net_pkt_write_u8(reply, ptr + addr_len)) {
		goto drop;
	}

	len++;

	skip = ptr - ptr_offset;
	if (skip) {
		/* Do not alter existed routes */
		if (net_pkt_write(reply, opt_data + offset, skip)) {
			goto drop;
		}

		offset += skip;
		len += skip;
	}

	if (net_pkt_write(reply, (void *)src, addr_len)) {
		goto drop;
	}

	len += addr_len;
	offset += addr_len;

	if (opt_len > offset) {
		if (net_pkt_write(reply, opt_data + offset, opt_len - offset)) {
			goto drop;
		}
	}

	len += opt_len - offset;

	net_pkt_set_ipv4_opts_len(reply, len);

	return 0;

drop:
	return -EINVAL;
}

/* TODO: Timestamp value should updated, as per RFC 791
 * Internet Timestamp. Timestamp value : 32-bit timestamp
 * in milliseconds since midnight UT.
 */
static int icmpv4_update_time_stamp(uint8_t *opt_data,
				   uint8_t opt_len,
				   struct net_pkt *reply,
				   const struct in_addr *src)
{
	uint8_t len = net_pkt_ipv4_opts_len(reply);
	uint8_t addr_len = sizeof(struct in_addr);
	uint8_t ptr_offset = 5U;
	uint8_t offset = 0U;
	uint8_t new_entry_len;
	uint8_t overflow;
	uint8_t flag;
	uint8_t skip;
	uint8_t ptr;

	if (net_pkt_write_u8(reply, NET_IPV4_OPTS_TS)) {
		goto drop;
	}

	len++;

	if (net_pkt_write_u8(reply, opt_len + 2U)) {
		goto drop;
	}

	len++;

	/* The Pointer is the number of octets from the beginning of
	 * this option to the end of timestamps plus one (i.e., it
	 * points to the octet beginning the space for next timestamp).
	 * The smallest legal value is 5.  The timestamp area is full
	 * when the pointer is greater than the length.
	 */
	ptr = opt_data[offset++];
	flag = opt_data[offset++];

	flag = flag & 0x0F;
	overflow = (flag & 0xF0) >> 4U;

	/* If the timestamp data area is already full (the pointer
	 * exceeds the length) the datagram is forwarded without
	 * inserting the timestamp, but the overflow count is
	 * incremented by one.
	 */
	if (ptr >= opt_len) {
		/* overflow count itself overflows, the original datagram
		 * is considered to be in error and is discarded.
		 */
		if (overflow == 0x0F) {
			goto drop;
		}

		/* No free entry to update Timestamp data */
		if (net_pkt_write_u8(reply, ptr)) {
			goto drop;
		}

		len++;

		overflow++;
		flag = (overflow << 4U) | flag;

		if (net_pkt_write_u8(reply, flag)) {
			goto drop;
		}

		len++;

		if (net_pkt_write(reply, opt_data + offset, opt_len)) {
			goto drop;
		}

		len += opt_len;

		net_pkt_set_ipv4_opts_len(reply, len);

		return 0;
	}

	switch (flag) {
	case NET_IPV4_TS_OPT_TS_ONLY:
		new_entry_len = sizeof(uint32_t);
		break;
	case NET_IPV4_TS_OPT_TS_ADDR:
		new_entry_len = addr_len + sizeof(uint32_t);
		break;
	case NET_IPV4_TS_OPT_TS_PRES: /* TODO */
	default:
		goto drop;
	}

	/* So, there is a free entry to update Timestamp */
	if (net_pkt_write_u8(reply, ptr + new_entry_len)) {
		goto drop;
	}

	len++;

	if (net_pkt_write_u8(reply, (overflow << 4) | flag)) {
		goto drop;
	}

	len++;

	skip = ptr - ptr_offset;
	if (skip) {
		/* Do not alter existed routes */
		if (net_pkt_write(reply, opt_data + offset, skip)) {
			goto drop;
		}

		len += skip;
		offset += skip;
	}

	switch (flag) {
	case NET_IPV4_TS_OPT_TS_ONLY:
		if (net_pkt_write_be32(reply, htons(k_uptime_get_32()))) {
			goto drop;
		}

		len += sizeof(uint32_t);

		offset += sizeof(uint32_t);

		break;
	case NET_IPV4_TS_OPT_TS_ADDR:
		if (net_pkt_write(reply, (void *)src, addr_len)) {
			goto drop;
		}

		len += addr_len;

		if (net_pkt_write_be32(reply, htons(k_uptime_get_32()))) {
			goto drop;
		}

		len += sizeof(uint32_t);

		offset += (addr_len + sizeof(uint32_t));

		break;
	}

	if (opt_len > offset) {
		if (net_pkt_write(reply, opt_data + offset, opt_len - offset)) {
			goto drop;
		}
	}

	len += opt_len - offset;

	net_pkt_set_ipv4_opts_len(reply, len);

	return 0;

drop:
	return -EINVAL;
}

static int icmpv4_reply_to_options(uint8_t opt_type,
				   uint8_t *opt_data,
				   uint8_t opt_len,
				   void *user_data)
{
	struct net_icmpv4_hdr_opts_data *ud =
		(struct net_icmpv4_hdr_opts_data *)user_data;

	if (opt_type == NET_IPV4_OPTS_RR) {
		return icmpv4_update_record_route(opt_data, opt_len,
						  ud->reply, ud->src);
	} else if (opt_type == NET_IPV4_OPTS_TS) {
		return icmpv4_update_time_stamp(opt_data, opt_len,
						ud->reply, ud->src);
	}

	return 0;
}

static int icmpv4_handle_header_options(struct net_pkt *pkt,
					struct net_pkt *reply,
					const struct in_addr *src)
{
	struct net_icmpv4_hdr_opts_data ud;
	uint8_t len;

	ud.reply = reply;
	ud.src = src;

	if (net_ipv4_parse_hdr_options(pkt, icmpv4_reply_to_options, &ud)) {
		return -EINVAL;
	}

	len = net_pkt_ipv4_opts_len(reply);

	/* IPv4 optional header part should ends in 32 bit boundary */
	if (len % 4U != 0U) {
		uint8_t i = 4U - (len % 4U);

		if (net_pkt_memset(reply, NET_IPV4_OPTS_NOP, i)) {
			return -EINVAL;
		}

		len += i;
	}

	/* Options are added now, update the header length. */
	net_pkt_set_ipv4_opts_len(reply, len);

	return 0;
}
#else
static int icmpv4_handle_header_options(struct net_pkt *pkt,
					struct net_pkt *reply,
					const struct in_addr *src)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(reply);
	ARG_UNUSED(src);

	return 0;
}
#endif

static int icmpv4_handle_echo_request(struct net_icmp_ctx *ctx,
				      struct net_pkt *pkt,
				      struct net_icmp_ip_hdr *hdr,
				      struct net_icmp_hdr *icmp_hdr,
				      void *user_data)
{
	struct net_pkt *reply = NULL;
	struct net_ipv4_hdr *ip_hdr = hdr->ipv4;
	const struct in_addr *src;
	int16_t payload_len;

	/* If interface can not select src address based on dst addr
	 * and src address is unspecified, drop the echo request.
	 */
	if (net_ipv4_is_addr_unspecified((struct in_addr *)ip_hdr->src)) {
		NET_DBG("DROP: src addr is unspecified");
		goto drop;
	}

	NET_DBG("Received Echo Request from %s to %s",
		net_sprint_ipv4_addr(&ip_hdr->src),
		net_sprint_ipv4_addr(&ip_hdr->dst));

	payload_len = net_pkt_get_len(pkt) -
		      net_pkt_ip_hdr_len(pkt) -
		      net_pkt_ipv4_opts_len(pkt) - NET_ICMPH_LEN;
	if (payload_len < NET_ICMPV4_UNUSED_LEN) {
		/* No identifier or sequence number present */
		goto drop;
	}

	reply = net_pkt_alloc_with_buffer(net_pkt_iface(pkt),
					  net_pkt_ipv4_opts_len(pkt) +
					  payload_len,
					  AF_INET, IPPROTO_ICMP,
					  PKT_WAIT_TIME);
	if (!reply) {
		NET_DBG("DROP: No buffer");
		goto drop;
	}

	if (net_ipv4_is_addr_mcast((struct in_addr *)ip_hdr->dst) ||
	    net_ipv4_is_addr_bcast(net_pkt_iface(pkt),
				   (struct in_addr *)ip_hdr->dst)) {
		src = net_if_ipv4_select_src_addr(net_pkt_iface(pkt),
						  (struct in_addr *)ip_hdr->dst);
	} else {
		src = (struct in_addr *)ip_hdr->dst;
	}

	net_pkt_set_ip_dscp(reply, net_pkt_ip_dscp(pkt));
	net_pkt_set_ip_ecn(reply, net_pkt_ip_ecn(pkt));

	if (net_ipv4_create(reply, src, (struct in_addr *)ip_hdr->src)) {
		goto drop;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4_HDR_OPTIONS)) {
		if (net_pkt_ipv4_opts_len(pkt) &&
		    icmpv4_handle_header_options(pkt, reply, src)) {
			goto drop;
		}
	}

	if (net_icmpv4_create(reply, NET_ICMPV4_ECHO_REPLY, 0) ||
	    net_pkt_copy(reply, pkt, payload_len)) {
		goto drop;
	}

	net_pkt_cursor_init(reply);
	net_ipv4_finalize(reply, IPPROTO_ICMP);

	NET_DBG("Sending Echo Reply from %s to %s",
		net_sprint_ipv4_addr(src),
		net_sprint_ipv4_addr(&ip_hdr->src));

	if (net_send_data(reply) < 0) {
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

int net_icmpv4_send_error(struct net_pkt *orig, uint8_t type, uint8_t code)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	int err = -EIO;
	struct net_ipv4_hdr *ip_hdr;
	struct net_pkt *pkt;
	size_t copy_len;

	net_pkt_cursor_init(orig);

	ip_hdr = (struct net_ipv4_hdr *)net_pkt_get_data(orig, &ipv4_access);
	if (!ip_hdr) {
		goto drop_no_pkt;
	}

	if (ip_hdr->proto == IPPROTO_ICMP) {
		NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmpv4_access,
						      struct net_icmp_hdr);
		struct net_icmp_hdr *icmp_hdr;

		icmp_hdr = (struct net_icmp_hdr *)net_pkt_get_data(
							orig, &icmpv4_access);
		if (!icmp_hdr || icmp_hdr->code < 8) {
			/* We must not send ICMP errors back */
			err = -EINVAL;
			goto drop_no_pkt;
		}
	}

	if (net_ipv4_is_addr_bcast(net_pkt_iface(orig),
				   (struct in_addr *)ip_hdr->dst)) {
		/* We should not send an error to packet that
		 * were sent to broadcast
		 */
		NET_DBG("Not sending error to bcast pkt from %s on proto %s",
			net_sprint_ipv4_addr(&ip_hdr->src),
			net_proto2str(AF_INET, ip_hdr->proto));
		goto drop_no_pkt;
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

	if (net_ipv4_create(pkt, (struct in_addr *)ip_hdr->dst,
			    (struct in_addr *)ip_hdr->src) ||
	    net_icmpv4_create(pkt, type, code) ||
	    net_pkt_memset(pkt, 0, NET_ICMPV4_UNUSED_LEN) ||
	    net_pkt_copy(pkt, orig, copy_len)) {
		goto drop;
	}

	net_pkt_cursor_init(pkt);
	net_ipv4_finalize(pkt, IPPROTO_ICMP);

	net_pkt_lladdr_dst(pkt)->addr = net_pkt_lladdr_src(orig)->addr;
	net_pkt_lladdr_dst(pkt)->len = net_pkt_lladdr_src(orig)->len;

	NET_DBG("Sending ICMPv4 Error Message type %d code %d from %s to %s",
		type, code,
		net_sprint_ipv4_addr(&ip_hdr->dst),
		net_sprint_ipv4_addr(&ip_hdr->src));

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

enum net_verdict net_icmpv4_input(struct net_pkt *pkt,
				  struct net_ipv4_hdr *ip_hdr)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access,
					      struct net_icmp_hdr);
	struct net_icmp_hdr *icmp_hdr;
	int ret;

	icmp_hdr = (struct net_icmp_hdr *)net_pkt_get_data(pkt, &icmp_access);
	if (!icmp_hdr) {
		NET_DBG("DROP: NULL ICMPv4 header");
		return NET_DROP;
	}

	if (net_if_need_calc_rx_checksum(net_pkt_iface(pkt))) {
		if (net_calc_chksum_icmpv4(pkt) != 0U) {
			NET_DBG("DROP: Invalid checksum");
			goto drop;
		}
	}

	if (net_ipv4_is_addr_bcast(net_pkt_iface(pkt),
				   (struct in_addr *)ip_hdr->dst) &&
	    (!IS_ENABLED(CONFIG_NET_ICMPV4_ACCEPT_BROADCAST) ||
	     icmp_hdr->type != NET_ICMPV4_ECHO_REQUEST)) {
		NET_DBG("DROP: broadcast pkt");
		goto drop;
	}

	net_pkt_acknowledge_data(pkt, &icmp_access);

	NET_DBG("ICMPv4 packet received type %d code %d",
		icmp_hdr->type, icmp_hdr->code);

	net_stats_update_icmp_recv(net_pkt_iface(pkt));

	ret = net_icmp_call_ipv4_handlers(pkt, ip_hdr, icmp_hdr);
	if (ret < 0) {
		NET_ERR("ICMPv4 handling failure (%d)", ret);
	}

	net_pkt_unref(pkt);

	return NET_OK;

drop:
	net_stats_update_icmp_drop(net_pkt_iface(pkt));

	return NET_DROP;
}

void net_icmpv4_init(void)
{
	static struct net_icmp_ctx ctx;
	int ret;

	ret = net_icmp_init_ctx(&ctx, NET_ICMPV4_ECHO_REQUEST, 0, icmpv4_handle_echo_request);
	if (ret < 0) {
		NET_ERR("Cannot register %s handler (%d)", STRINGIFY(NET_ICMPV4_ECHO_REQUEST),
			ret);
	}
}
