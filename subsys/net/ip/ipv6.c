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

#include <logging/log.h>
LOG_MODULE_REGISTER(net_ipv6, CONFIG_NET_IPV6_LOG_LEVEL);

#include <errno.h>
#include <stdlib.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_stats.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
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

	ipv6_hdr = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &ipv6_access);
	if (!ipv6_hdr) {
		return -ENOBUFS;
	}

	ipv6_hdr->vtc     = 0x60;
	ipv6_hdr->tcflow  = 0U;
	ipv6_hdr->flow    = 0U;
	ipv6_hdr->len     = 0U;
	ipv6_hdr->nexthdr = 0U;

	/* User can tweak the default hop limit if needed */
	ipv6_hdr->hop_limit = net_pkt_ipv6_hop_limit(pkt);
	if (ipv6_hdr->hop_limit == 0U) {
		ipv6_hdr->hop_limit =
			net_if_ipv6_get_hop_limit(net_pkt_iface(pkt));
	}

	net_ipaddr_copy(&ipv6_hdr->dst, dst);
	net_ipaddr_copy(&ipv6_hdr->src, src);

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
					       uint16_t length)
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
		if (net_ipv6_is_addr_mcast(&hdr->dst)) {
			break;
		}

		__fallthrough;
	case 0x80:
		net_icmpv6_send_error(pkt, NET_ICMPV6_PARAM_PROBLEM,
				      NET_ICMPV6_PARAM_PROB_OPTION,
				      (uint32_t)length);
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

	if (net_pkt_read_u8(pkt, (uint8_t *)&exthdr_len)) {
		return -ENOBUFS;
	}

	exthdr_len = exthdr_len * 8U + 8;
	if (exthdr_len > pkt_len) {
		NET_DBG("Corrupted packet, extension header %d too long "
			"(max %d bytes)", exthdr_len, pkt_len);
		return -EINVAL;
	}

	length += 2U;

	while (length < exthdr_len) {
		uint8_t opt_type, opt_len;

		/* Each extension option has type and length */
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
			length++;
			break;
		case NET_IPV6_EXT_HDR_OPT_PADN:
			NET_DBG("PADN option");
			length += opt_len + 2;

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
							opt_type, length)) {
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

	route = net_route_add(iface, addr, prefix_len, addr);

	NET_DBG("%s route to %s/%d iface %p", route ? "Add" : "Cannot add",
		log_strdup(net_sprint_ipv6_addr(addr)), prefix_len, iface);

	return route;
}
#endif /* CONFIG_NET_ROUTE */

static void ipv6_no_route_info(struct net_pkt *pkt,
			       struct in6_addr *src,
			       struct in6_addr *dst)
{
	NET_DBG("Will not route pkt %p ll src %s to dst %s between interfaces",
		pkt, log_strdup(net_sprint_ipv6_addr(src)),
		log_strdup(net_sprint_ipv6_addr(dst)));
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
		found = net_route_get_info(NULL, &hdr->dst, &route,
					   &nexthop);
	} else {
		found = net_route_get_info(net_pkt_iface(pkt),
					   &hdr->dst, &route, &nexthop);
	}

	if (found) {
		int ret;

		if (IS_ENABLED(CONFIG_NET_ROUTING) &&
		    (net_ipv6_is_ll_addr(&hdr->src) ||
		     net_ipv6_is_ll_addr(&hdr->dst))) {
			/* RFC 4291 ch 2.5.6 */
			ipv6_no_route_info(pkt, &hdr->src, &hdr->dst);
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

			add_route(net_pkt_orig_iface(pkt), &hdr->src, 128);
		}

		ret = net_route_packet(pkt, nexthop);
		if (ret < 0) {
			NET_DBG("Cannot re-route pkt %p via %s "
				"at iface %p (%d)",
				pkt, log_strdup(net_sprint_ipv6_addr(nexthop)),
				net_pkt_iface(pkt), ret);
		} else {
			return NET_OK;
		}
	} else {
		struct net_if *iface = NULL;
		int ret;

		if (net_if_ipv6_addr_onlink(&iface, &hdr->dst)) {
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
			log_strdup(net_sprint_ipv6_addr(&hdr->dst)), pkt);
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
	if (net_ipv6_is_addr_mcast(&hdr->src)  ||
	      net_ipv6_is_addr_mcast_iface(&hdr->dst) ||
	       net_ipv6_is_ll_addr(&hdr->src)) {
		return NET_CONTINUE;
	}

	routed = net_route_mcast_forward_packet(pkt, hdr);

	if (routed < 0) {
		return NET_DROP;
	}
#endif /*CONFIG_NET_ROUTE_MCAST*/
	return NET_CONTINUE;
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
	uint8_t nexthdr, next_nexthdr, prev_hdr_offset;
	union net_proto_header proto_hdr;
	struct net_ipv6_hdr *hdr;
	struct net_if_mcast_addr *if_mcast_addr;
	union net_ip_header ip;
	int pkt_len;

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
		log_strdup(net_sprint_ipv6_addr(&hdr->src)),
		log_strdup(net_sprint_ipv6_addr(&hdr->dst)));

	if (net_ipv6_is_addr_unspecified(&hdr->src)) {
		NET_DBG("DROP: src addr is %s", "unspecified");
		goto drop;
	}

	if (net_ipv6_is_addr_mcast(&hdr->src) ||
	    net_ipv6_is_addr_mcast_scope(&hdr->dst, 0)) {
		NET_DBG("DROP: multicast packet");
		goto drop;
	}

	if (!is_loopback) {
		if (net_ipv6_is_addr_loopback(&hdr->dst) ||
		    net_ipv6_is_addr_loopback(&hdr->src)) {
			NET_DBG("DROP: ::1 packet");
			goto drop;
		}

		if (net_ipv6_is_addr_mcast_iface(&hdr->dst) ||
		    (net_ipv6_is_addr_mcast_group(
			    &hdr->dst, net_ipv6_unspecified_address()) &&
		     (net_ipv6_is_addr_mcast_site(&hdr->dst) ||
		      net_ipv6_is_addr_mcast_org(&hdr->dst)))) {
			NET_DBG("DROP: invalid scope multicast packet");
			goto drop;
		}
	}

	/* Check extension headers */
	net_pkt_set_ipv6_next_hdr(pkt, hdr->nexthdr);
	net_pkt_set_ipv6_ext_len(pkt, 0);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_ipv6_hop_limit(pkt, NET_IPV6_HDR(pkt)->hop_limit);
	net_pkt_set_family(pkt, PF_INET6);

	if (IS_ENABLED(CONFIG_NET_ROUTE_MCAST) &&
		net_ipv6_is_addr_mcast(&hdr->dst)) {
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

	if (!net_ipv6_is_addr_mcast(&hdr->dst)) {
		if (!net_ipv6_is_my_addr(&hdr->dst)) {
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
		    net_ipv6_is_ll_addr(&hdr->src) &&
		    !net_if_ipv6_addr_lookup_by_iface(pkt_iface, &hdr->dst)) {
			ipv6_no_route_info(pkt, &hdr->src, &hdr->dst);
			goto drop;
		}
	}

	if (net_ipv6_is_addr_mcast(&hdr->dst) &&
	    !(net_ipv6_is_addr_mcast_iface(&hdr->dst) ||
	      net_ipv6_is_addr_mcast_link_all_nodes(&hdr->dst))) {
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
		if_mcast_addr = net_if_ipv6_maddr_lookup(&hdr->dst, &pkt_iface);

		if (!if_mcast_addr ||
		    !net_if_ipv6_maddr_is_joined(if_mcast_addr)) {
			NET_DBG("DROP: packet for unjoined multicast address");
			goto drop;
		}
	}

	net_pkt_acknowledge_data(pkt, &ipv6_access);

	nexthdr = hdr->nexthdr;
	prev_hdr_offset = (uint8_t *)&hdr->nexthdr - (uint8_t *)hdr;

	while (!net_ipv6_is_nexthdr_upper_layer(nexthdr)) {
		int exthdr_len;

		NET_DBG("IPv6 next header %d", nexthdr);

		if (net_pkt_read_u8(pkt, &next_nexthdr)) {
			goto drop;
		}

		switch (nexthdr) {
		case NET_IPV6_NEXTHDR_HBHO:
			if (ext_bitmap & NET_IPV6_EXT_HDR_BITMAP_HBHO) {
				NET_ERR("DROP: multiple hop-by-hop");
				goto drop;
			}

			/* HBH option needs to be the first one */
			if (nexthdr != hdr->nexthdr) {
				goto bad_hdr;
			}

			ext_bitmap |= NET_IPV6_EXT_HDR_BITMAP_HBHO;

			break;

		case NET_IPV6_NEXTHDR_DESTO:
			if (ext_bitmap & NET_IPV6_EXT_HDR_BITMAP_DESTO2) {
				/* DESTO option cannot appear more than twice */
				goto bad_hdr;
			}

			if (ext_bitmap & NET_IPV6_EXT_HDR_BITMAP_DESTO1) {
				ext_bitmap |= NET_IPV6_EXT_HDR_BITMAP_DESTO2;
			} else {
				ext_bitmap |= NET_IPV6_EXT_HDR_BITMAP_DESTO1;
			}

			break;

		case NET_IPV6_NEXTHDR_FRAG:
			if (IS_ENABLED(CONFIG_NET_IPV6_FRAGMENT)) {
				net_pkt_set_ipv6_hdr_prev(pkt,
							  prev_hdr_offset);
				net_pkt_set_ipv6_fragment_start(
					pkt,
					net_pkt_get_current_offset(pkt) - 1);
				return net_ipv6_handle_fragment_hdr(pkt, hdr,
								    nexthdr);
			}

			goto bad_hdr;

		case NET_IPV6_NEXTHDR_NONE:
			/* There is nothing after this header (see RFC 2460,
			 * ch 4.7), so we can drop the packet now.
			 * This is not an error case so do not update drop
			 * statistics.
			 */
			return NET_DROP;

		default:
			goto bad_hdr;
		}

		exthdr_len = ipv6_handle_ext_hdr_options(pkt, hdr, pkt_len);
		if (exthdr_len < 0) {
			goto drop;
		}

		ext_len += exthdr_len;
		nexthdr = next_nexthdr;
	}

	net_pkt_set_ipv6_ext_len(pkt, ext_len);

	switch (nexthdr) {
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
	}

	if (verdict == NET_DROP) {
		goto drop;
	} else if (nexthdr == IPPROTO_ICMPV6) {
		return verdict;
	}

	ip.ipv6 = hdr;

	verdict = net_conn_input(pkt, &ip, nexthdr, &proto_hdr);
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
