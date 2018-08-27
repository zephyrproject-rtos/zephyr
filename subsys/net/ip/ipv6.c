/** @file
 * @brief IPv6 related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_IPV6)
#define SYS_LOG_DOMAIN "net/ipv6"
#define NET_LOG_ENABLED 1

/* By default this prints too much data, set the value to 1 to see
 * neighbor cache contents.
 */
#define NET_DEBUG_NBR 0
#endif

#include <errno.h>
#include <stdlib.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_stats.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
#include <net/tcp.h>
#include "net_private.h"
#include "connection.h"
#include "icmpv6.h"
#include "udp_internal.h"
#include "tcp_internal.h"
#include "ipv6.h"
#include "nbr.h"
#include "6lo.h"
#include "route.h"
#include "rpl.h"
#include "net_stats.h"

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

/* IPv6 wildcard and loopback address defined by RFC2553 */
const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;

const struct in6_addr *net_ipv6_unspecified_address(void)
{
	return &in6addr_any;
}

struct net_pkt *net_ipv6_create(struct net_pkt *pkt,
				const struct in6_addr *src,
				const struct in6_addr *dst,
				struct net_if *iface,
				u8_t next_header_proto)
{
	struct net_buf *header;

	header = net_pkt_get_frag(pkt, NET_BUF_TIMEOUT);
	if (!header) {
		return NULL;
	}

	net_pkt_frag_insert(pkt, header);

	NET_IPV6_HDR(pkt)->vtc = 0x60;
	NET_IPV6_HDR(pkt)->tcflow = 0;
	NET_IPV6_HDR(pkt)->flow = 0;

	NET_IPV6_HDR(pkt)->nexthdr = 0;

	/* User can tweak the default hop limit if needed */
	NET_IPV6_HDR(pkt)->hop_limit = net_pkt_ipv6_hop_limit(pkt);
	if (NET_IPV6_HDR(pkt)->hop_limit == 0) {
		NET_IPV6_HDR(pkt)->hop_limit =
					net_if_ipv6_get_hop_limit(iface);
	}

	net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst, dst);
	net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src, src);

	net_pkt_set_ipv6_ext_len(pkt, 0);
	NET_IPV6_HDR(pkt)->nexthdr = next_header_proto;

	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_family(pkt, AF_INET6);

	net_buf_add(header, sizeof(struct net_ipv6_hdr));

	return pkt;
}

int net_ipv6_finalize(struct net_pkt *pkt, u8_t next_header_proto)
{
	/* Set the length of the IPv6 header */
	size_t total_len;
	int ret;

#if defined(CONFIG_NET_UDP) && defined(CONFIG_NET_RPL_INSERT_HBH_OPTION)
	if (next_header_proto != IPPROTO_TCP &&
	    next_header_proto != IPPROTO_ICMPV6) {
		/* Check if we need to add RPL header to sent UDP packet. */
		if (net_rpl_insert_header(pkt) < 0) {
			NET_DBG("RPL HBHO insert failed");
			return -EINVAL;
		}
	}
#endif
	net_pkt_compact(pkt);

	total_len = net_pkt_get_len(pkt) - sizeof(struct net_ipv6_hdr);

	NET_IPV6_HDR(pkt)->len = htons(total_len);

#if defined(CONFIG_NET_UDP)
	if (next_header_proto == IPPROTO_UDP &&
	    net_if_need_calc_tx_checksum(net_pkt_iface(pkt))) {
		net_udp_set_chksum(pkt, pkt->frags);
	} else
#endif

#if defined(CONFIG_NET_TCP)
	if (next_header_proto == IPPROTO_TCP &&
	    net_if_need_calc_tx_checksum(net_pkt_iface(pkt))) {
		net_tcp_set_chksum(pkt, pkt->frags);
	} else
#endif

	if (next_header_proto == IPPROTO_ICMPV6) {
		ret = net_icmpv6_set_chksum(pkt);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static inline enum net_verdict process_icmpv6_pkt(struct net_pkt *pkt,
						  struct net_ipv6_hdr *ipv6)
{
	struct net_icmp_hdr icmp_hdr;
	int ret;

	ret = net_icmpv6_get_hdr(pkt, &icmp_hdr);
	if (ret < 0) {
		NET_DBG("NULL ICMPv6 header - dropping");
		return NET_DROP;
	}

	NET_DBG("ICMPv6 %s received type %d code %d",
		net_icmpv6_type2str(icmp_hdr.type), icmp_hdr.type,
		icmp_hdr.code);

	return net_icmpv6_input(pkt, icmp_hdr.type, icmp_hdr.code);
}

static inline struct net_pkt *check_unknown_option(struct net_pkt *pkt,
						   u8_t opt_type,
						   u16_t length)
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
	NET_DBG("Unknown option %d (0x%02x) MSB %d", opt_type, opt_type,
		opt_type >> 6);

	switch (opt_type & 0xc0) {
	case 0x00:
		break;
	case 0x40:
		return NULL;
	case 0xc0:
		if (net_is_ipv6_addr_mcast(&NET_IPV6_HDR(pkt)->dst)) {
			return NULL;
		}
		/* passthrough */
	case 0x80:
		net_icmpv6_send_error(pkt, NET_ICMPV6_PARAM_PROBLEM,
				      NET_ICMPV6_PARAM_PROB_OPTION,
				      (u32_t)length);
		return NULL;
	}

	return pkt;
}

static inline struct net_buf *handle_ext_hdr_options(struct net_pkt *pkt,
						     struct net_buf *frag,
						     int total_len,
						     u16_t len,
						     u16_t offset,
						     u16_t *pos,
						     enum net_verdict *verdict)
{
	u8_t opt_type, opt_len;
	u16_t length = 0, loc;
#if defined(CONFIG_NET_RPL)
	bool result;
#endif

	if (len > total_len) {
		NET_DBG("Corrupted packet, extension header %d too long "
			"(max %d bytes)", len, total_len);
		*verdict = NET_DROP;
		return NULL;
	}

	length += 2;

	/* Each extension option has type and length */
	frag = net_frag_read_u8(frag, offset, &loc, &opt_type);
	if (!frag && loc == 0xffff) {
		goto drop;
	}

	if (opt_type != NET_IPV6_EXT_HDR_OPT_PAD1) {
		frag = net_frag_read_u8(frag, loc, &loc, &opt_len);
		if (!frag && loc == 0xffff) {
			goto drop;
		}
	}

	while (frag && (length < len)) {
		switch (opt_type) {
		case NET_IPV6_EXT_HDR_OPT_PAD1:
			length++;
			break;
		case NET_IPV6_EXT_HDR_OPT_PADN:
			NET_DBG("PADN option");
			length += opt_len + 2;
			loc += opt_len + 2;
			break;
#if defined(CONFIG_NET_RPL)
		case NET_IPV6_EXT_HDR_OPT_RPL:
			NET_DBG("Processing RPL option");
			frag = net_rpl_verify_header(pkt, frag, loc, &loc,
						     &result);
			if (!result) {
				NET_DBG("RPL option error, packet dropped");
				goto drop;
			}

			if (!frag && *pos == 0xffff) {
				goto drop;
			}

			*verdict = NET_CONTINUE;
			return frag;
#endif
		default:
			if (!check_unknown_option(pkt, opt_type, length)) {
				goto drop;
			}

			length += opt_len + 2;

			/* No need to +2 here as loc already contains option
			 * header len.
			 */
			loc += opt_len;

			break;
		}

		if (length >= len) {
			break;
		}

		frag = net_frag_read_u8(frag, loc, &loc, &opt_type);
		if (!frag && loc == 0xffff) {
			goto drop;
		}

		if (opt_type != NET_IPV6_EXT_HDR_OPT_PAD1) {
			frag = net_frag_read_u8(frag, loc, &loc, &opt_len);
			if (!frag && loc == 0xffff) {
				goto drop;
			}
		}
	}

	if (length != len) {
		goto drop;
	}

	*pos = loc;

	*verdict = NET_CONTINUE;
	return frag;

drop:
	*verdict = NET_DROP;
	return NULL;
}

static inline bool is_upper_layer_protocol_header(u8_t proto)
{
	return (proto == IPPROTO_ICMPV6 || proto == IPPROTO_UDP ||
		proto == IPPROTO_TCP);
}

#if defined(CONFIG_NET_ROUTE)
static struct net_route_entry *add_route(struct net_if *iface,
					 struct in6_addr *addr,
					 u8_t prefix_len)
{
	struct net_route_entry *route;

	route = net_route_lookup(iface, addr);
	if (route) {
		return route;
	}

	route = net_route_add(iface, addr, prefix_len, addr);

	NET_DBG("%s route to %s/%d iface %p", route ? "Add" : "Cannot add",
		net_sprint_ipv6_addr(addr), prefix_len, iface);

	return route;
}
#endif /* CONFIG_NET_ROUTE */

static void no_route_info(struct net_pkt *pkt,
			  struct in6_addr *src,
			  struct in6_addr *dst)
{
	NET_DBG("Will not route pkt %p ll src %s to dst %s between interfaces",
		pkt, net_sprint_ipv6_addr(src), net_sprint_ipv6_addr(dst));
}

#if defined(CONFIG_NET_ROUTE)
static enum net_verdict route_ipv6_packet(struct net_pkt *pkt,
					  struct net_ipv6_hdr *hdr)
{
	struct net_route_entry *route;
	struct in6_addr *nexthop;
	bool found;

	/* Check if the packet can be routed */
	if (IS_ENABLED(CONFIG_NET_ROUTING)) {
		found = net_route_get_info(NULL, &hdr->dst, &route,
					   &nexthop);
	} else {
		found = net_route_get_info(net_pkt_iface(pkt),
					   &hdr->dst, &route, &nexthop);
	}

	if (found) {
		int ret;

		if (IS_ENABLED(CONFIG_NET_ROUTING) &&
		    (net_is_ipv6_ll_addr(&hdr->src) ||
		     net_is_ipv6_ll_addr(&hdr->dst))) {
			/* RFC 4291 ch 2.5.6 */
			no_route_info(pkt, &hdr->src, &hdr->dst);
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
				  &NET_IPV6_HDR(pkt)->src, 128);
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
		NET_DBG("No route to %s pkt %p dropped",
			net_sprint_ipv6_addr(&hdr->dst), pkt);
	}

drop:
	return NET_DROP;
}
#endif /* CONFIG_NET_ROUTE */

enum net_verdict net_ipv6_process_pkt(struct net_pkt *pkt)
{
	struct net_ipv6_hdr *hdr = NET_IPV6_HDR(pkt);
	int real_len = net_pkt_get_len(pkt);
	int pkt_len = ntohs(hdr->len) + sizeof(*hdr);
	struct net_buf *frag;
	u8_t start_of_ext, prev_hdr;
	u8_t next, next_hdr;
	u8_t first_option;
	u16_t offset;
	u16_t length;
	u16_t total_len = 0;
	u8_t ext_bitmap;

	if (real_len != pkt_len) {
		NET_DBG("IPv6 packet size %d pkt len %d", pkt_len, real_len);
		net_stats_update_ipv6_drop(net_pkt_iface(pkt));
		goto drop;
	}

	NET_DBG("IPv6 packet len %d received from %s to %s", real_len,
		net_sprint_ipv6_addr(&hdr->src),
		net_sprint_ipv6_addr(&hdr->dst));

	if (net_is_ipv6_addr_mcast(&hdr->src)) {
		NET_DBG("Dropping src multicast packet");
		net_stats_update_ipv6_drop(net_pkt_iface(pkt));
		goto drop;
	}

	if (!net_is_my_ipv6_addr(&hdr->dst) &&
	    !net_is_my_ipv6_maddr(&hdr->dst) &&
	    !net_is_ipv6_addr_mcast(&hdr->dst) &&
	    !net_is_ipv6_addr_loopback(&hdr->dst)) {
#if defined(CONFIG_NET_ROUTE)
		enum net_verdict verdict;

		verdict = route_ipv6_packet(pkt, hdr);
		if (verdict == NET_OK) {
			return NET_OK;
		}
#else /* CONFIG_NET_ROUTE */
		NET_DBG("IPv6 packet in pkt %p not for me", pkt);
#endif /* CONFIG_NET_ROUTE */

		net_stats_update_ipv6_drop(net_pkt_iface(pkt));
		goto drop;
	}

	/* If we receive a packet with ll source address fe80: and destination
	 * address is one of ours, and if the packet would cross interface
	 * boundary, then drop the packet. RFC 4291 ch 2.5.6
	 */
	if (IS_ENABLED(CONFIG_NET_ROUTING) &&
	    net_is_ipv6_ll_addr(&hdr->src) &&
	    !net_is_ipv6_addr_mcast(&hdr->dst) &&
	    !net_if_ipv6_addr_lookup_by_iface(net_pkt_iface(pkt),
					      &hdr->dst)) {
		no_route_info(pkt, &hdr->src, &hdr->dst);

		net_stats_update_ipv6_drop(net_pkt_iface(pkt));
		goto drop;
	}

	/* Check extension headers */
	net_pkt_set_next_hdr(pkt, &hdr->nexthdr);
	net_pkt_set_ipv6_ext_len(pkt, 0);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_ipv6_hop_limit(pkt, NET_IPV6_HDR(pkt)->hop_limit);

	/* Fast path for main upper layer protocols. The handling of extension
	 * headers can be slow so do this checking here. There cannot
	 * be any extension headers after the upper layer protocol header.
	 */
	next = *(net_pkt_next_hdr(pkt));
	if (is_upper_layer_protocol_header(next)) {
		goto upper_proto;
	}

	/* Go through the extensions */
	frag = pkt->frags;
	next = hdr->nexthdr;
	first_option = next;
	length = 0;
	ext_bitmap = 0;
	start_of_ext = 0;
	offset = sizeof(struct net_ipv6_hdr);
	prev_hdr = &NET_IPV6_HDR(pkt)->nexthdr - &NET_IPV6_HDR(pkt)->vtc;

	while (frag) {
		enum net_verdict verdict;

		if (is_upper_layer_protocol_header(next)) {
			NET_DBG("IPv6 next header %d", next);
			goto upper_proto;
		}

		if (!start_of_ext) {
			start_of_ext = offset;
		}

		frag = net_frag_read_u8(frag, offset, &offset, &next_hdr);
		if (!frag) {
			goto drop;
		}

		verdict = NET_OK;

		NET_DBG("IPv6 next header %d", next);

		switch (next) {
		case NET_IPV6_NEXTHDR_NONE:
			/* There is nothing after this header (see RFC 2460,
			 * ch 4.7), so we can drop the packet now.
			 * This is not an error case so do not update drop
			 * statistics.
			 */
			goto drop;

		case NET_IPV6_NEXTHDR_HBHO:
			if (ext_bitmap & NET_IPV6_EXT_HDR_BITMAP_HBHO) {
				NET_ERR("Dropping packet with multiple HBHO");
				goto drop;
			}

			frag = net_frag_read_u8(frag, offset, &offset,
						(u8_t *)&length);
			if (!frag) {
				goto drop;
			}

			length = length * 8 + 8;
			total_len += length;

			/* HBH option needs to be the first one */
			if (first_option != NET_IPV6_NEXTHDR_HBHO) {
				goto bad_hdr;
			}

			ext_bitmap |= NET_IPV6_EXT_HDR_BITMAP_HBHO;

			frag = handle_ext_hdr_options(pkt, frag, real_len,
						      length, offset, &offset,
						      &verdict);
			break;

#if defined(CONFIG_NET_IPV6_FRAGMENT)
		case NET_IPV6_NEXTHDR_FRAG:
			net_pkt_set_ipv6_hdr_prev(pkt, prev_hdr);

			net_pkt_set_ipv6_fragment_start(pkt,
							sizeof(struct
							       net_ipv6_hdr) +
							total_len);

			total_len += 8;
			return net_ipv6_handle_fragment_hdr(pkt, frag, real_len,
							    offset, &offset,
							    next_hdr);
#endif
		default:
			goto bad_hdr;
		}

		if (verdict == NET_DROP) {
			goto drop;
		}

		prev_hdr = start_of_ext;
		next = next_hdr;
	}

upper_proto:

	net_pkt_set_ipv6_ext_len(pkt, total_len);

	switch (next) {
	case IPPROTO_ICMPV6:
		return process_icmpv6_pkt(pkt, hdr);
	case IPPROTO_UDP:
#if defined(CONFIG_NET_UDP)
		return net_conn_input(IPPROTO_UDP, pkt);
#else
		return NET_DROP;
#endif
	case IPPROTO_TCP:
#if defined(CONFIG_NET_TCP)
		return net_conn_input(IPPROTO_TCP, pkt);
#else
		return NET_DROP;
#endif
	}

drop:
	return NET_DROP;

bad_hdr:
	/* Send error message about parameter problem (RFC 2460)
	 */
	net_icmpv6_send_error(pkt, NET_ICMPV6_PARAM_PROBLEM,
			      NET_ICMPV6_PARAM_PROB_NEXTHEADER,
			      offset - 1);

	NET_DBG("Unknown next header type");
	net_stats_update_ip_errors_protoerr(net_pkt_iface(pkt));

	return NET_DROP;
}

void net_ipv6_init(void)
{
	net_ipv6_nbr_init();

#if defined(CONFIG_NET_IPV6_MLD)
	net_ipv6_mld_init();
#endif
}
