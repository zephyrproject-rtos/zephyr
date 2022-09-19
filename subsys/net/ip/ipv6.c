/** @file
 * @brief IPv6 related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* By default this prints too much data, set the value to 1 to see
 * neighbor cache contents.
 */
#define NET_DEBUG_NBR 0

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ipv6, CONFIG_NET_IPV6_LOG_LEVEL);

#include <errno.h>
#include <stdlib.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/virtual.h>
#include "net_private.h"
#include "connection.h"
#include "icmpv6.h"
#include "udp_internal.h"
#include "tcp_internal.h"
#include "ipv6.h"
#include "nbr.h"
#include "6lo.h"
#include "route.h"
#include "net_stats.h"

BUILD_ASSERT(sizeof(struct in6_addr) == NET_IPV6_ADDR_SIZE);

/* Timeout value to be used when allocating net buffer during various
 * neighbor discovery procedures.
 */
#define ND_NET_BUF_TIMEOUT K_MSEC(100)

/* Timeout for various buffer allocations in this file. */
#define NET_BUF_TIMEOUT K_MSEC(50)

/* Maximum reachable time value specified in RFC 4861 section
 * 6.2.1. Router Configuration Variables, AdvReachableTime
 */
#define MAX_REACHABLE_TIME 3600000

int net_ipv6_create(struct net_pkt *pkt,
		    const struct in6_addr *src,
		    const struct in6_addr *dst)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	struct net_ipv6_hdr *ipv6_hdr;
	uint8_t tc = 0;

	ipv6_hdr = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &ipv6_access);
	if (!ipv6_hdr) {
		return -ENOBUFS;
	}

	if (IS_ENABLED(CONFIG_NET_IP_DSCP_ECN)) {
		net_ipv6_set_dscp(&tc, net_pkt_ip_dscp(pkt));
		net_ipv6_set_ecn(&tc, net_pkt_ip_ecn(pkt));
	}

	ipv6_hdr->vtc     = 0x60 | ((tc >> 4) & 0x0F);
	ipv6_hdr->tcflow  = (tc << 4) & 0xF0;
	ipv6_hdr->flow    = 0U;
	ipv6_hdr->len     = 0U;
	ipv6_hdr->nexthdr = 0U;

	/* User can tweak the default hop limit if needed */
	ipv6_hdr->hop_limit = net_pkt_ipv6_hop_limit(pkt);
	if (ipv6_hdr->hop_limit == 0U) {
		ipv6_hdr->hop_limit =
			net_if_ipv6_get_hop_limit(net_pkt_iface(pkt));
	}

	net_ipv6_addr_copy_raw(ipv6_hdr->dst, (uint8_t *)dst);
	net_ipv6_addr_copy_raw(ipv6_hdr->src, (uint8_t *)src);

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_ipv6_ext_len(pkt, 0);

	return net_pkt_set_data(pkt, &ipv6_access);
}

int net_ipv6_finalize(struct net_pkt *pkt, uint8_t next_header_proto)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	struct net_ipv6_hdr *ipv6_hdr;

	net_pkt_set_overwrite(pkt, true);

	ipv6_hdr = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &ipv6_access);
	if (!ipv6_hdr) {
		return -ENOBUFS;
	}

	ipv6_hdr->len = htons(net_pkt_get_len(pkt) -
			      sizeof(struct net_ipv6_hdr));

	if (net_pkt_ipv6_next_hdr(pkt) != 255U) {
		ipv6_hdr->nexthdr = net_pkt_ipv6_next_hdr(pkt);
	} else {
		ipv6_hdr->nexthdr = next_header_proto;
	}

	net_pkt_set_data(pkt, &ipv6_access);

	if (net_pkt_ipv6_next_hdr(pkt) != 255U &&
	    net_pkt_skip(pkt, net_pkt_ipv6_ext_len(pkt))) {
		return -ENOBUFS;
	}

	if (IS_ENABLED(CONFIG_NET_UDP) &&
	    next_header_proto == IPPROTO_UDP) {
		return net_udp_finalize(pkt);
	} else if (IS_ENABLED(CONFIG_NET_TCP) &&
		   next_header_proto == IPPROTO_TCP) {
		return net_tcp_finalize(pkt);
	} else if (next_header_proto == IPPROTO_ICMPV6) {
		return net_icmpv6_finalize(pkt);
	}

	return 0;
}

static inline bool ipv6_drop_on_unknown_option(struct net_pkt *pkt,
					       struct net_ipv6_hdr *hdr,
					       uint8_t opt_type,
					       uint16_t opt_type_offset)
{
	/* RFC 2460 chapter 4.2 tells how to handle the unknown
	 * options by the two highest order bits of the option:
	 *
	 * 00: Skip over this option and continue processing the header.
	 * 01: Discard the packet.
	 * 10: Discard the packet and, regardless of whether or not the
	 *     packet's Destination Address was a multicast address,
	 *     send an ICMP Parameter Problem, Code 2, message to the packet's
	 *     Source Address, pointing to the unrecognized Option Type.
	 * 11: Discard the packet and, only if the packet's Destination
	 *     Address was not a multicast address, send an ICMP Parameter
	 *     Problem, Code 2, message to the packet's Source Address,
	 *     pointing to the unrecognized Option Type.
	 */
	NET_DBG("Unknown option %d (0x%02x) MSB %d - 0x%02x",
		opt_type, opt_type, opt_type >> 6, opt_type & 0xc0);

	switch (opt_type & 0xc0) {
	case 0x00:
		return false;
	case 0x40:
		break;
	case 0xc0:
		if (net_ipv6_is_addr_mcast((struct in6_addr *)hdr->dst)) {
			break;
		}

		__fallthrough;
	case 0x80:
		net_icmpv6_send_error(pkt, NET_ICMPV6_PARAM_PROBLEM,
				      NET_ICMPV6_PARAM_PROB_OPTION,
				      (uint32_t)opt_type_offset);
		break;
	}

	return true;
}

static inline int ipv6_handle_ext_hdr_options(struct net_pkt *pkt,
					      struct net_ipv6_hdr *hdr,
					      uint16_t pkt_len)
{
	uint16_t exthdr_len = 0U;
	uint16_t length = 0U;

	{
		uint8_t val = 0U;

		if (net_pkt_read_u8(pkt, &val)) {
			return -ENOBUFS;
		}
		exthdr_len = val * 8U + 8;
	}

	if (exthdr_len > pkt_len) {
		NET_DBG("Corrupted packet, extension header %d too long "
			"(max %d bytes)", exthdr_len, pkt_len);
		return -EINVAL;
	}

	length += 2U;

	while (length < exthdr_len) {
		uint16_t opt_type_offset;
		uint8_t opt_type, opt_len;

		opt_type_offset = net_pkt_get_current_offset(pkt);

		/* Each extension option has type and length - except
		 * Pad1 which has only a type without any length
		 */
		if (net_pkt_read_u8(pkt, &opt_type)) {
			return -ENOBUFS;
		}

		if (opt_type != NET_IPV6_EXT_HDR_OPT_PAD1) {
			if (net_pkt_read_u8(pkt, &opt_len)) {
				return -ENOBUFS;
			}
		}

		switch (opt_type) {
		case NET_IPV6_EXT_HDR_OPT_PAD1:
			NET_DBG("PAD1 option");
			length++;
			break;
		case NET_IPV6_EXT_HDR_OPT_PADN:
			NET_DBG("PADN option");
			length += opt_len + 2;
			net_pkt_skip(pkt, opt_len);
			break;
		default:
			/* Make sure that the option length is not too large.
			 * The former 1 + 1 is the length of extension type +
			 * length fields.
			 * The latter 1 + 1 is the length of the sub-option
			 * type and length fields.
			 */
			if (opt_len > (exthdr_len - (1 + 1 + 1 + 1))) {
				return -EINVAL;
			}

			if (ipv6_drop_on_unknown_option(pkt, hdr,
							opt_type, opt_type_offset)) {
				return -ENOTSUP;
			}

			if (net_pkt_skip(pkt, opt_len)) {
				return -ENOBUFS;
			}

			length += opt_len + 2;

			break;
		}
	}

	return exthdr_len;
}

#if defined(CONFIG_NET_ROUTE)
static struct net_route_entry *add_route(struct net_if *iface,
					 struct in6_addr *addr,
					 uint8_t prefix_len)
{
	struct net_route_entry *route;

	route = net_route_lookup(iface, addr);
	if (route) {
		return route;
	}

	route = net_route_add(iface, addr, prefix_len, addr,
			      NET_IPV6_ND_INFINITE_LIFETIME,
			      NET_ROUTE_PREFERENCE_LOW);

	NET_DBG("%s route to %s/%d iface %p", route ? "Add" : "Cannot add",
		net_sprint_ipv6_addr(addr), prefix_len, iface);

	return route;
}
#endif /* CONFIG_NET_ROUTE */

static void ipv6_no_route_info(struct net_pkt *pkt,
			       struct in6_addr *src,
			       struct in6_addr *dst)
{
	NET_DBG("Will not route pkt %p ll src %s to dst %s between interfaces",
		pkt, net_sprint_ipv6_addr(src),
		net_sprint_ipv6_addr(dst));
}

#if defined(CONFIG_NET_ROUTE)
static enum net_verdict ipv6_route_packet(struct net_pkt *pkt,
					  struct net_ipv6_hdr *hdr)
{
	struct net_route_entry *route;
	struct in6_addr *nexthop;
	bool found;

	/* Check if the packet can be routed */
	if (IS_ENABLED(CONFIG_NET_ROUTING)) {
		found = net_route_get_info(NULL, (struct in6_addr *)hdr->dst,
					   &route, &nexthop);
	} else {
		found = net_route_get_info(net_pkt_iface(pkt),
					   (struct in6_addr *)hdr->dst,
					   &route, &nexthop);
	}

	if (found) {
		int ret;

		if (IS_ENABLED(CONFIG_NET_ROUTING) &&
		    (net_ipv6_is_ll_addr((struct in6_addr *)hdr->src) ||
		     net_ipv6_is_ll_addr((struct in6_addr *)hdr->dst))) {
			/* RFC 4291 ch 2.5.6 */
			ipv6_no_route_info(pkt, (struct in6_addr *)hdr->src,
					   (struct in6_addr *)hdr->dst);
			goto drop;
		}

		/* Used when detecting if the original link
		 * layer address length is changed or not.
		 */
		net_pkt_set_orig_iface(pkt, net_pkt_iface(pkt));

		if (route) {
			net_pkt_set_iface(pkt, route->iface);
		}

		if (IS_ENABLED(CONFIG_NET_ROUTING) &&
		    net_pkt_orig_iface(pkt) != net_pkt_iface(pkt)) {
			/* If the route interface to destination is
			 * different than the original route, then add
			 * route to original source.
			 */
			NET_DBG("Route pkt %p from %p to %p",
				pkt, net_pkt_orig_iface(pkt),
				net_pkt_iface(pkt));

			add_route(net_pkt_orig_iface(pkt),
				  (struct in6_addr *)hdr->src, 128);
		}

		ret = net_route_packet(pkt, nexthop);
		if (ret < 0) {
			NET_DBG("Cannot re-route pkt %p via %s "
				"at iface %p (%d)",
				pkt, net_sprint_ipv6_addr(nexthop),
				net_pkt_iface(pkt), ret);
		} else {
			return NET_OK;
		}
	} else {
		struct net_if *iface = NULL;
		int ret;

		if (net_if_ipv6_addr_onlink(&iface, (struct in6_addr *)hdr->dst)) {
			ret = net_route_packet_if(pkt, iface);
			if (ret < 0) {
				NET_DBG("Cannot re-route pkt %p "
					"at iface %p (%d)",
					pkt, net_pkt_iface(pkt), ret);
			} else {
				return NET_OK;
			}
		}

		NET_DBG("No route to %s pkt %p dropped",
			net_sprint_ipv6_addr(&hdr->dst), pkt);
	}

drop:
	return NET_DROP;
}
#else
static inline enum net_verdict ipv6_route_packet(struct net_pkt *pkt,
						 struct net_ipv6_hdr *hdr)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(hdr);

	NET_DBG("DROP: Packet %p not for me", pkt);

	return NET_DROP;
}

#endif /* CONFIG_NET_ROUTE */


static enum net_verdict ipv6_forward_mcast_packet(struct net_pkt *pkt,
						 struct net_ipv6_hdr *hdr)
{
#if defined(CONFIG_NET_ROUTE_MCAST)
	int routed;

	/* check if routing loop could be created or if the destination is of
	 * interface local scope or if from link local source
	 */
	if (net_ipv6_is_addr_mcast((struct in6_addr *)hdr->src)  ||
	      net_ipv6_is_addr_mcast_iface((struct in6_addr *)hdr->dst) ||
	       net_ipv6_is_ll_addr((struct in6_addr *)hdr->src)) {
		return NET_CONTINUE;
	}

	routed = net_route_mcast_forward_packet(pkt, hdr);

	if (routed < 0) {
		return NET_DROP;
	}
#endif /*CONFIG_NET_ROUTE_MCAST*/
	return NET_CONTINUE;
}

static uint8_t extension_to_bitmap(uint8_t header, uint8_t ext_bitmap)
{
	switch (header) {
	case NET_IPV6_NEXTHDR_HBHO:
		return NET_IPV6_EXT_HDR_BITMAP_HBHO;
	case NET_IPV6_NEXTHDR_DESTO:
		/* Destination header can appears twice */
		if (ext_bitmap & NET_IPV6_EXT_HDR_BITMAP_DESTO1) {
			return NET_IPV6_EXT_HDR_BITMAP_DESTO2;
		}
		return NET_IPV6_EXT_HDR_BITMAP_DESTO1;
	case NET_IPV6_NEXTHDR_ROUTING:
		return NET_IPV6_EXT_HDR_BITMAP_ROUTING;
	case NET_IPV6_NEXTHDR_FRAG:
		return NET_IPV6_EXT_HDR_BITMAP_FRAG;
	default:
		return 0;
	}
}

enum net_verdict net_ipv6_input(struct net_pkt *pkt, bool is_loopback)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
	NET_PKT_DATA_ACCESS_DEFINE(tcp_access, struct net_tcp_hdr);
	struct net_if *pkt_iface = net_pkt_iface(pkt);
	enum net_verdict verdict = NET_DROP;
	int real_len = net_pkt_get_len(pkt);
	uint8_t ext_bitmap = 0U;
	uint16_t ext_len = 0U;
	uint8_t current_hdr, nexthdr, prev_hdr_offset;
	union net_proto_header proto_hdr;
	struct net_ipv6_hdr *hdr;
	struct net_if_mcast_addr *if_mcast_addr;
	union net_ip_header ip;
	int pkt_len;

#if defined(CONFIG_NET_L2_VIRTUAL)
	struct net_pkt_cursor hdr_start;

	net_pkt_cursor_backup(pkt, &hdr_start);
#endif

	net_stats_update_ipv6_recv(pkt_iface);

	hdr = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &ipv6_access);
	if (!hdr) {
		NET_DBG("DROP: no buffer");
		goto drop;
	}

	pkt_len = ntohs(hdr->len) + sizeof(struct net_ipv6_hdr);
	if (real_len < pkt_len) {
		NET_DBG("DROP: pkt len per hdr %d != pkt real len %d",
			pkt_len, real_len);
		goto drop;
	} else if (real_len > pkt_len) {
		net_pkt_update_length(pkt, pkt_len);
	}

	NET_DBG("IPv6 packet len %d received from %s to %s", pkt_len,
		net_sprint_ipv6_addr(&hdr->src),
		net_sprint_ipv6_addr(&hdr->dst));

	if (net_ipv6_is_addr_unspecified((struct in6_addr *)hdr->src)) {
		NET_DBG("DROP: src addr is %s", "unspecified");
		goto drop;
	}

	if (net_ipv6_is_addr_mcast((struct in6_addr *)hdr->src) ||
	    net_ipv6_is_addr_mcast_scope((struct in6_addr *)hdr->dst, 0)) {
		NET_DBG("DROP: multicast packet");
		goto drop;
	}

	if (!is_loopback) {
		if (net_ipv6_is_addr_loopback((struct in6_addr *)hdr->dst) ||
		    net_ipv6_is_addr_loopback((struct in6_addr *)hdr->src)) {
			NET_DBG("DROP: ::1 packet");
			goto drop;
		}

		if (net_ipv6_is_addr_mcast_iface((struct in6_addr *)hdr->dst) ||
		    (net_ipv6_is_addr_mcast_group(
			    (struct in6_addr *)hdr->dst,
			    net_ipv6_unspecified_address()) &&
		     (net_ipv6_is_addr_mcast_site((struct in6_addr *)hdr->dst) ||
		      net_ipv6_is_addr_mcast_org((struct in6_addr *)hdr->dst)))) {
			NET_DBG("DROP: invalid scope multicast packet");
			goto drop;
		}
	}

	/* Reconstruct TC field. */

	if (IS_ENABLED(CONFIG_NET_IP_DSCP_ECN)) {
		uint8_t tc = ((hdr->vtc << 4) & 0xF0) | ((hdr->tcflow >> 4) & 0x0F);

		net_pkt_set_ip_dscp(pkt, net_ipv6_get_dscp(tc));
		net_pkt_set_ip_ecn(pkt, net_ipv6_get_ecn(tc));
	}

	/* Check extension headers */
	net_pkt_set_ipv6_next_hdr(pkt, hdr->nexthdr);
	net_pkt_set_ipv6_ext_len(pkt, 0);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_ipv6_hop_limit(pkt, NET_IPV6_HDR(pkt)->hop_limit);
	net_pkt_set_family(pkt, PF_INET6);

	if (IS_ENABLED(CONFIG_NET_ROUTE_MCAST) &&
		net_ipv6_is_addr_mcast((struct in6_addr *)hdr->dst)) {
		/* If the packet is a multicast packet and multicast routing
		 * is activated, we give the packet to the routing engine.
		 *
		 * But we only drop the packet if an error occurs, otherwise
		 * it might be eminent to respond on the packet on application
		 * layer.
		 */
		if (ipv6_forward_mcast_packet(pkt, hdr) == NET_DROP) {
			goto drop;
		}
	}

	if (!net_ipv6_is_addr_mcast((struct in6_addr *)hdr->dst)) {
		if (!net_ipv6_is_my_addr((struct in6_addr *)hdr->dst)) {
			if (ipv6_route_packet(pkt, hdr) == NET_OK) {
				return NET_OK;
			}

			goto drop;
		}

		/* If we receive a packet with ll source address fe80: and
		 * destination address is one of ours, and if the packet would
		 * cross interface boundary, then drop the packet.
		 * RFC 4291 ch 2.5.6
		 */
		if (IS_ENABLED(CONFIG_NET_ROUTING) &&
		    net_ipv6_is_ll_addr((struct in6_addr *)hdr->src) &&
		    !net_if_ipv6_addr_lookup_by_iface(
				pkt_iface, (struct in6_addr *)hdr->dst)) {
			ipv6_no_route_info(pkt, (struct in6_addr *)hdr->src,
					   (struct in6_addr *)hdr->dst);
			goto drop;
		}
	}

	if (net_ipv6_is_addr_mcast((struct in6_addr *)hdr->dst) &&
	    !(net_ipv6_is_addr_mcast_iface((struct in6_addr *)hdr->dst) ||
	      net_ipv6_is_addr_mcast_link_all_nodes((struct in6_addr *)hdr->dst))) {
		/* If we receive a packet with a interface-local or
		 * link-local all-nodes multicast destination address we
		 * always have to pass it to the upper layer.
		 *
		 * For all other destination multicast addresses we have to
		 * check if one of the joined multicast groups on the
		 * originating interface of the packet matches. Otherwise the
		 * packet will be dropped.
		 * RFC4291 ch 2.7.1, ch 2.8
		 */
		if_mcast_addr = net_if_ipv6_maddr_lookup(
				    (struct in6_addr *)hdr->dst, &pkt_iface);

		if (!if_mcast_addr ||
		    !net_if_ipv6_maddr_is_joined(if_mcast_addr)) {
			NET_DBG("DROP: packet for unjoined multicast address");
			goto drop;
		}
	}

	net_pkt_acknowledge_data(pkt, &ipv6_access);

	current_hdr = hdr->nexthdr;
	ext_bitmap = extension_to_bitmap(current_hdr, ext_bitmap);
	/* Offset of "nexthdr" in the IPv6 header */
	prev_hdr_offset = (uint8_t *)&hdr->nexthdr - (uint8_t *)hdr;
	net_pkt_set_ipv6_hdr_prev(pkt, prev_hdr_offset);

	while (!net_ipv6_is_nexthdr_upper_layer(current_hdr)) {
		int exthdr_len;
		uint8_t ext_bit;

		NET_DBG("IPv6 next header %d", current_hdr);

		if (current_hdr == NET_IPV6_NEXTHDR_NONE) {
			/* There is nothing after this header (see RFC 2460,
			 * ch 4.7), so we can drop the packet now.
			 * This is not an error case so do not update drop
			 * statistics.
			 */
			return NET_DROP;
		}

		/* Offset of "nexthdr" in the Extension Header */
		prev_hdr_offset = net_pkt_get_current_offset(pkt);

		if (net_pkt_read_u8(pkt, &nexthdr)) {
			goto drop;
		}

		/* Detect duplicated Extension headers */
		ext_bit = extension_to_bitmap(nexthdr, ext_bitmap);
		if (ext_bit & ext_bitmap) {
			goto bad_hdr;
		}
		ext_bitmap |= ext_bit;

		/* Make sure that nexthdr is valid, reject the Extension Header early otherwise.
		 * This is also important so that the "pointer" field in the ICMPv6 error
		 * message points to the "nexthdr" field.
		 */
		switch (nexthdr) {
		case NET_IPV6_NEXTHDR_HBHO:
			/* Hop-by-hop header can appear only once and must appear right after
			 * the IPv6 header. Consequently the "nexthdr" field of an Extension
			 * Header can never be an HBH option.
			 */
			goto bad_hdr;

		case NET_IPV6_NEXTHDR_DESTO:
		case NET_IPV6_NEXTHDR_FRAG:
		case NET_IPV6_NEXTHDR_NONE:
			/* Valid values */
			break;

		default:
			if (net_ipv6_is_nexthdr_upper_layer(nexthdr)) {
				break;
			}
			goto bad_hdr;
		}

		/* Process the current Extension Header */
		switch (current_hdr) {
		case NET_IPV6_NEXTHDR_HBHO:
		case NET_IPV6_NEXTHDR_DESTO:
			/* Process options below */
			break;

		case NET_IPV6_NEXTHDR_FRAG:
			if (IS_ENABLED(CONFIG_NET_IPV6_FRAGMENT)) {
				net_pkt_set_ipv6_fragment_start(
					pkt,
					net_pkt_get_current_offset(pkt) - 1);
				return net_ipv6_handle_fragment_hdr(pkt, hdr,
								    current_hdr);
			}

			goto bad_hdr;

		default:
			/* Unsupported */
			goto bad_hdr;
		}

		exthdr_len = ipv6_handle_ext_hdr_options(pkt, hdr, pkt_len);
		if (exthdr_len < 0) {
			goto drop;
		}

		ext_len += exthdr_len;
		current_hdr = nexthdr;
		/* Save the offset to "nexthdr" in case we need to overwrite it
		 * when processing a fragment header
		 */
		net_pkt_set_ipv6_hdr_prev(pkt, prev_hdr_offset);
	}

	net_pkt_set_ipv6_ext_len(pkt, ext_len);

	switch (current_hdr) {
	case IPPROTO_ICMPV6:
		verdict = net_icmpv6_input(pkt, hdr);
		break;
	case IPPROTO_TCP:
		proto_hdr.tcp = net_tcp_input(pkt, &tcp_access);
		if (proto_hdr.tcp) {
			verdict = NET_OK;
		}
		break;
	case IPPROTO_UDP:
		proto_hdr.udp = net_udp_input(pkt, &udp_access);
		if (proto_hdr.udp) {
			verdict = NET_OK;
		}
		break;
#if defined(CONFIG_NET_L2_VIRTUAL)
	case IPPROTO_IPV6:
	case IPPROTO_IPIP: {
		struct net_addr remote_addr;

		remote_addr.family = AF_INET6;
		net_ipv6_addr_copy_raw((uint8_t *)&remote_addr.in6_addr, hdr->src);

		/* Get rid of the old IP header */
		net_pkt_cursor_restore(pkt, &hdr_start);
		net_pkt_pull(pkt, net_pkt_ip_hdr_len(pkt) +
			     net_pkt_ipv6_ext_len(pkt));

		return net_virtual_input(pkt_iface, &remote_addr, pkt);
	}
#endif
	}

	if (verdict == NET_DROP) {
		goto drop;
	} else if (current_hdr == IPPROTO_ICMPV6) {
		return verdict;
	}

	ip.ipv6 = hdr;

	verdict = net_conn_input(pkt, &ip, current_hdr, &proto_hdr);
	if (verdict != NET_DROP) {
		return verdict;
	}

drop:
	net_stats_update_ipv6_drop(pkt_iface);
	return NET_DROP;

bad_hdr:
	/* Send error message about parameter problem (RFC 2460) */
	net_icmpv6_send_error(pkt, NET_ICMPV6_PARAM_PROBLEM,
			      NET_ICMPV6_PARAM_PROB_NEXTHEADER,
			      net_pkt_get_current_offset(pkt) - 1);

	NET_DBG("DROP: Unknown/wrong nexthdr type");
	net_stats_update_ip_errors_protoerr(pkt_iface);

	return NET_DROP;
}

void net_ipv6_init(void)
{
	net_ipv6_nbr_init();

#if defined(CONFIG_NET_IPV6_MLD)
	net_ipv6_mld_init();
#endif
}
