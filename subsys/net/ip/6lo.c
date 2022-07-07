/** @file
 * @brief 6lopan related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_6lo, CONFIG_NET_6LO_LOG_LEVEL);

#include <errno.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/udp.h>

#include "net_private.h"
#include "6lo.h"
#include "6lo_private.h"

#if defined(CONFIG_NET_6LO_CONTEXT)
struct net_6lo_context {
	struct in6_addr prefix;
	struct net_if *iface;
	uint16_t lifetime;
	uint8_t is_used		: 1;
	uint8_t compress	: 1;
	uint8_t cid		: 4;
	uint8_t unused		: 2;
};

static inline uint8_t get_6co_compress(struct net_icmpv6_nd_opt_6co *opt)
{
	return (opt->flag & 0x10) >> 4;
}

static inline uint8_t get_6co_cid(struct net_icmpv6_nd_opt_6co *opt)
{
	return opt->flag & 0x0F;
}

static struct net_6lo_context ctx_6co[CONFIG_NET_MAX_6LO_CONTEXTS];
#endif

static const uint8_t udp_nhc_inline_size_table[] = {4, 3, 3, 1};

static const uint8_t tf_inline_size_table[] = {4, 3, 1, 0};
/* The first bit of the index is SAC        |  SAC=0   |  SAC=1   |*/
static const uint8_t sa_inline_size_table[] = {16, 8, 2, 0, 0, 8, 2, 0};

/* The first bit is M, the second DAC
 *	| M=0 DAC=0 | M=0 DAC=1 | M=1 DAC=0  | M=1 DAC=1 (DAM always 00)
 */
static const uint8_t da_inline_size_table[] = {
	16, 8, 2, 0, 0, 8, 2, 0, 16, 6, 4, 1, 6
	};

static int get_udp_nhc_inlined_size(uint8_t nhc)
{
	int size = 0;

	if ((nhc & 0xF8) != NET_6LO_NHC_UDP_BARE) {
		NET_DBG("UDP NHC dispatch doesn't match");
		return 0;
	}

	if (!(nhc & NET_6LO_NHC_UDP_CHECKSUM)) {
		size += 2U;
	}

	size += udp_nhc_inline_size_table[(nhc & NET_6LO_NHC_UDP_PORT_MASK)];

	NET_DBG("Size of inlined UDP HDR data: %d", size);

	return size;
}

static int get_ihpc_inlined_size(uint16_t iphc)
{
	int size = 0;

	if (((iphc >> 8) & NET_6LO_DISPATCH_IPHC_MASK) !=
	    NET_6LO_DISPATCH_IPHC) {
		NET_DBG("IPHC dispatch doesn't match");
		return -1;
	}

	size += tf_inline_size_table[(iphc & NET_6LO_IPHC_TF_MASK) >>
				     NET_6LO_IPHC_TF_POS];

	if (!(iphc & NET_6LO_IPHC_NH_MASK)) {
		size += 1U;
	}

	if (!(iphc & NET_6LO_IPHC_HLIM_MASK)) {
		size += 1U;
	}

	if (iphc & NET_6LO_IPHC_CID_MASK) {
		size += 1U;
	}

	size += sa_inline_size_table[(iphc & NET_6LO_IPHC_SA_MASK) >>
				      NET_6LO_IPHC_SAM_POS];

	size += da_inline_size_table[(iphc & NET_6LO_IPHC_DA_MASK) >>
				      NET_6LO_IPHC_DAM_POS];

	NET_DBG("Size of inlined IP HDR data: %d", size);

	return size;
}

/* TODO: Unicast-Prefix based IPv6 Multicast(dst) address compression
 *       Mesh header compression
 */

static inline bool net_6lo_ll_prefix_padded_with_zeros(struct in6_addr *addr)
{
	return (net_ipv6_is_ll_addr(addr) &&
		(UNALIGNED_GET(&addr->s6_addr16[1]) == 0x00) &&
		(UNALIGNED_GET(&addr->s6_addr32[1]) == 0x00));
}

static inline bool net_6lo_addr_16_bit_compressible(struct in6_addr *addr)
{
	return ((UNALIGNED_GET(&addr->s6_addr32[2]) == htonl(0xFF)) &&
		 (UNALIGNED_GET(&addr->s6_addr16[6]) == htons(0xFE00)));
}

static inline bool net_6lo_maddr_8_bit_compressible(struct in6_addr *addr)
{
	return ((addr->s6_addr[1] == 0x02) &&
		 (UNALIGNED_GET(&addr->s6_addr16[1]) == 0x00) &&
		 (UNALIGNED_GET(&addr->s6_addr32[1]) == 0x00) &&
		 (UNALIGNED_GET(&addr->s6_addr32[2]) == 0x00) &&
		 (addr->s6_addr[14] == 0x00));
}

static inline bool net_6lo_maddr_32_bit_compressible(struct in6_addr *addr)
{
	return ((UNALIGNED_GET(&addr->s6_addr32[1]) == 0x00) &&
		 (UNALIGNED_GET(&addr->s6_addr32[2]) == 0x00) &&
		 (addr->s6_addr[12] == 0x00));
}

static inline bool net_6lo_maddr_48_bit_compressible(struct in6_addr *addr)
{
	return ((UNALIGNED_GET(&addr->s6_addr32[1]) == 0x00) &&
		 (UNALIGNED_GET(&addr->s6_addr16[4]) == 0x00) &&
		 (addr->s6_addr[10] == 0x00));
}

#if defined(CONFIG_NET_6LO_CONTEXT)
/* RFC 6775, 4.2, 5.4.2, 5.4.3 and 7.2*/
static inline void set_6lo_context(struct net_if *iface, uint8_t index,
				   struct net_icmpv6_nd_opt_6co *context)

{
	ctx_6co[index].is_used = true;
	ctx_6co[index].iface = iface;

	/*TODO: Start timer */
	ctx_6co[index].lifetime = context->lifetime;
	ctx_6co[index].compress = get_6co_compress(context);
	ctx_6co[index].cid = get_6co_cid(context);

	net_ipv6_addr_copy_raw((uint8_t *)&ctx_6co[index].prefix, context->prefix);
}

void net_6lo_set_context(struct net_if *iface,
			 struct net_icmpv6_nd_opt_6co *context)
{
	int unused = -1;
	uint8_t i;

	/* If the context information already exists, update or remove
	 * as per data.
	 */
	for (i = 0U; i < CONFIG_NET_MAX_6LO_CONTEXTS; i++) {
		if (!ctx_6co[i].is_used) {
			unused = i;
			continue;
		}

		if (ctx_6co[i].iface == iface &&
		    ctx_6co[i].cid == get_6co_cid(context)) {
			/* Remove if lifetime is zero */
			if (!context->lifetime) {
				ctx_6co[i].is_used = false;
				return;
			}

			/* Update the context */
			set_6lo_context(iface, i, context);
			return;
		}
	}

	/* Cache the context information. */
	if (unused != -1) {
		set_6lo_context(iface, unused, context);
		return;
	}

	NET_DBG("Either no free slots in the table or exceeds limit");
}

/* Get the context by matching cid */
static inline struct net_6lo_context *
get_6lo_context_by_cid(struct net_if *iface, uint8_t cid)
{
	uint8_t i;

	for (i = 0U; i < CONFIG_NET_MAX_6LO_CONTEXTS; i++) {
		if (!ctx_6co[i].is_used) {
			continue;
		}

		if (ctx_6co[i].iface == iface && ctx_6co[i].cid == cid) {
			return &ctx_6co[i];
		}
	}

	return NULL;
}

/* Get the context by addr */
static inline struct net_6lo_context *
get_6lo_context_by_addr(struct net_if *iface, struct in6_addr *addr)
{
	uint8_t i;

	for (i = 0U; i < CONFIG_NET_MAX_6LO_CONTEXTS; i++) {
		if (!ctx_6co[i].is_used) {
			continue;
		}

		if (ctx_6co[i].iface == iface &&
		    !memcmp(ctx_6co[i].prefix.s6_addr, addr->s6_addr, 8)) {
			return &ctx_6co[i];
		}
	}

	return NULL;
}

#endif

/* Helper routine to compress Traffic class and Flow label */
/* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |Version| Traffic Class |           Flow Label                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * version: 4 bits, Traffic Class: 8 bits, Flow label: 20 bits
 * The Traffic Class field in the IPv6 header comprises 6 bits of
 * Diffserv extension [RFC2474] and 2 bits of Explicit Congestion
 * Notification (ECN) [RFC3168]
 */

/* IPHC (compressed) format of traffic class is ECN, DSCP but original
 * IPv6 traffic class format is DSCP, ECN.
 * DSCP(6), ECN(2).
 */
static uint8_t *compress_tfl(struct net_ipv6_hdr *ipv6, uint8_t *inline_ptr,
			  uint16_t *iphc)
{
	uint8_t tcl;

	tcl = ((ipv6->vtc & 0x0F) << 4) | ((ipv6->tcflow & 0xF0) >> 4);
	tcl = (tcl << 6) | (tcl >> 2);   /* ECN(2), DSCP(6) */

	if (((ipv6->tcflow & 0x0F) == 0U) && (ipv6->flow == 0U)) {
		if (((ipv6->vtc & 0x0F) == 0U) && ((ipv6->tcflow & 0xF0) == 0U)) {
			NET_DBG("Traffic class and Flow label elided");

			/* Traffic class and Flow label elided */
			*iphc |= NET_6LO_IPHC_TF_11;
		} else {
			NET_DBG("Flow label elided");

			/* Flow label elided */
			*iphc |= NET_6LO_IPHC_TF_10;

			inline_ptr -= sizeof(tcl);
			*inline_ptr = tcl;
		}
	} else {
		if (((ipv6->vtc & 0x0F) == 0U) && (ipv6->tcflow & 0x30)) {
			NET_DBG("ECN + 2-bit Pad + Flow Label, DSCP is elided");

			/* ECN + 2-bit Pad + Flow Label, DSCP is elided.*/
			*iphc |= NET_6LO_IPHC_TF_01;

			inline_ptr -= sizeof(ipv6->flow);
			memmove(inline_ptr, &ipv6->flow, sizeof(ipv6->flow));

			inline_ptr -= sizeof(uint8_t);
			*inline_ptr = (tcl & 0xC0) | (ipv6->tcflow & 0x0F);
		} else {
			NET_DBG("ECN + DSCP + 4-bit Pad + Flow Label");

			/* ECN + DSCP + 4-bit Pad + Flow Label */
			*iphc |= NET_6LO_IPHC_TF_00;

			inline_ptr -= sizeof(ipv6->flow);
			memmove(inline_ptr, &ipv6->flow, sizeof(ipv6->flow));

			inline_ptr -= sizeof(uint8_t);
			*inline_ptr = ipv6->tcflow & 0x0F;
			inline_ptr -= sizeof(tcl);
			*inline_ptr = tcl;
		}
	}

	return inline_ptr;
}

/* Helper to compress Hop limit */
static uint8_t *compress_hoplimit(struct net_ipv6_hdr *ipv6, uint8_t *inline_ptr,
			       uint16_t *iphc)
{
	/* Hop Limit */
	switch (ipv6->hop_limit) {
	case 1:
		NET_DBG("HLIM compressed (1)");
		*iphc |= NET_6LO_IPHC_HLIM1;
		break;
	case 64:
		NET_DBG("HLIM compressed (64)");
		*iphc |= NET_6LO_IPHC_HLIM64;
		break;
	case 255:
		NET_DBG("HLIM compressed (255)");
		*iphc |= NET_6LO_IPHC_HLIM255;
		break;
	default:
		inline_ptr -= sizeof(ipv6->hop_limit);
		*inline_ptr = ipv6->hop_limit;
		break;
	}

	return inline_ptr;
}

/* Helper to compress Next header */
static uint8_t *compress_nh(struct net_ipv6_hdr *ipv6, uint8_t *inline_ptr,
			 uint16_t *iphc)
{
	/* Next header */
	if (ipv6->nexthdr == IPPROTO_UDP) {
		*iphc |= NET_6LO_IPHC_NH_1;
	} else {
		inline_ptr -= sizeof(ipv6->nexthdr);
		*inline_ptr = ipv6->nexthdr;
	}

	return inline_ptr;
}

/* Helpers to compress Source Address */
static uint8_t *compress_sa(struct net_ipv6_hdr *ipv6, struct net_pkt *pkt,
			 uint8_t *inline_ptr, uint16_t *iphc)
{
	NET_ASSERT(net_pkt_lladdr_src(pkt)->addr);

	/* Address is fully elided */
	if (net_ipv6_addr_based_on_ll((struct in6_addr *)ipv6->src,
				      net_pkt_lladdr_src(pkt))) {
		NET_DBG("SAM_11 src address is fully elided");

		*iphc |= NET_6LO_IPHC_SAM_11;
		return inline_ptr;
	}

	/* Following 64 bits are 0000:00ff:fe00:XXXX */
	if (net_6lo_addr_16_bit_compressible((struct in6_addr *)ipv6->src)) {
		NET_DBG("SAM_10 src addr 16 bit compressible");
		*iphc |= NET_6LO_IPHC_SAM_10;

		inline_ptr -= sizeof(uint16_t);
		memmove(inline_ptr, &ipv6->src[14], sizeof(uint16_t));

		return inline_ptr;
	}

	NET_DBG("SAM_01 src 64 bits are inlined");
	/* Remaining 64 bits are in-line */
	*iphc |= NET_6LO_IPHC_SAM_01;

	inline_ptr -= 8U;
	memmove(inline_ptr, &ipv6->src[8], 8U);

	return inline_ptr;
}

static uint8_t *set_sa_inline(struct net_ipv6_hdr *ipv6, uint8_t *inline_ptr,
			   uint16_t *iphc)
{
	*iphc |= NET_6LO_IPHC_SAM_00;
	inline_ptr -= 16U;
	memmove(inline_ptr, &ipv6->src[0], 16U);
	return inline_ptr;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
static uint8_t *compress_sa_ctx(struct net_ipv6_hdr *ipv6, uint8_t *inline_ptr,
			     struct net_pkt *pkt, uint16_t *iphc,
			     struct net_6lo_context *src)
{
	NET_ASSERT(net_pkt_lladdr_src(pkt)->addr);

	NET_DBG("SAC_1 src address context based");
	*iphc |= NET_6LO_IPHC_SAC_1;

	if (net_ipv6_addr_based_on_ll((struct in6_addr *)ipv6->src,
				      net_pkt_lladdr_src(pkt))) {
		NET_DBG("SAM_11 src address is fully elided");

		/* Address is fully elided */
		*iphc |= NET_6LO_IPHC_SAM_11;
		return inline_ptr;
	}

	/* Following 64 bits are 0000:00ff:fe00:XXXX */
	if (net_6lo_addr_16_bit_compressible((struct in6_addr *)ipv6->src)) {
		NET_DBG("SAM_10 src addr 16 bit compressible");

		*iphc |= NET_6LO_IPHC_SAM_10;

		inline_ptr -= sizeof(uint16_t);
		memmove(inline_ptr, &ipv6->src[14], sizeof(uint16_t));
		return inline_ptr;
	}

	NET_DBG("SAM_01 src remaining 64 bits are inlined");

	/* Remaining 64 bits are in-line */
	*iphc |= NET_6LO_IPHC_SAM_01;

	inline_ptr -= 8U;
	memmove(inline_ptr, &ipv6->src[8], 8U);

	return inline_ptr;
}
#endif

/* Helpers to compress Destination Address */
static uint8_t *compress_da_mcast(struct net_ipv6_hdr *ipv6, uint8_t *inline_ptr,
			       uint16_t *iphc)
{
	*iphc |= NET_6LO_IPHC_M_1;

	NET_DBG("M_1 dst is mcast");

	if (net_6lo_maddr_8_bit_compressible((struct in6_addr *)ipv6->dst)) {
		NET_DBG("DAM_11 dst maddr 8 bit compressible");

		/* last byte */
		*iphc |= NET_6LO_IPHC_DAM_11;

		inline_ptr -= sizeof(uint8_t);
		memmove(inline_ptr, &ipv6->dst[15], sizeof(uint8_t));

		return inline_ptr;
	}

	if (net_6lo_maddr_32_bit_compressible((struct in6_addr *)ipv6->dst)) {
		NET_DBG("DAM_10 4 bytes: 2nd byte + last three bytes");

		/* 4 bytes: 2nd byte + last three bytes */
		*iphc |= NET_6LO_IPHC_DAM_10;

		inline_ptr -= 3U;
		memmove(inline_ptr, &ipv6->dst[13], 3U);

		inline_ptr -= sizeof(uint8_t);
		memmove(inline_ptr, &ipv6->dst[1], sizeof(uint8_t));

		return inline_ptr;
	}

	if (net_6lo_maddr_48_bit_compressible((struct in6_addr *)ipv6->dst)) {
		NET_DBG("DAM_01 6 bytes: 2nd byte + last five bytes");

		/* 6 bytes: 2nd byte + last five bytes */
		*iphc |= NET_6LO_IPHC_DAM_01;

		inline_ptr -= 5U;
		memmove(inline_ptr, &ipv6->dst[11], 5U);

		inline_ptr -= sizeof(uint8_t);
		memmove(inline_ptr, &ipv6->dst[1], sizeof(uint8_t));

		return inline_ptr;
	}

	NET_DBG("DAM_00 dst complete addr inlined");

	/* complete address NET_6LO_IPHC_DAM_00 */
	inline_ptr -= 16U;
	memmove(inline_ptr, &ipv6->dst[0], 16U);

	return inline_ptr;
}

static uint8_t *compress_da(struct net_ipv6_hdr *ipv6, struct net_pkt *pkt,
			 uint8_t *inline_ptr, uint16_t *iphc)
{
	NET_ASSERT(net_pkt_lladdr_dst(pkt)->addr);

	/* Address is fully elided */
	if (net_ipv6_addr_based_on_ll((struct in6_addr *)ipv6->dst,
				      net_pkt_lladdr_dst(pkt))) {
		NET_DBG("DAM_11 dst addr fully elided");

		*iphc |= NET_6LO_IPHC_DAM_11;
		return inline_ptr;
	}

	/* Following 64 bits are 0000:00ff:fe00:XXXX */
	if (net_6lo_addr_16_bit_compressible((struct in6_addr *)ipv6->dst)) {
		NET_DBG("DAM_10 dst addr 16 bit compressible");

		*iphc |= NET_6LO_IPHC_DAM_10;

		inline_ptr -= sizeof(uint16_t);
		memmove(inline_ptr, &ipv6->dst[14], sizeof(uint16_t));
		return inline_ptr;
	}

	NET_DBG("DAM_01 remaining 64 bits are inlined");

	/* Remaining 64 bits are in-line */
	*iphc |= NET_6LO_IPHC_DAM_01;

	inline_ptr -= 8U;
	memmove(inline_ptr, &ipv6->dst[8], 8U);

	return inline_ptr;
}

static uint8_t *set_da_inline(struct net_ipv6_hdr *ipv6, uint8_t *inline_ptr,
			   uint16_t *iphc)
{
	*iphc |= NET_6LO_IPHC_DAM_00;
	inline_ptr -= 16U;
	memmove(inline_ptr, &ipv6->dst[0], 16U);
	return inline_ptr;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
static uint8_t *compress_da_ctx(struct net_ipv6_hdr *ipv6, uint8_t *inline_ptr,
			     struct net_pkt *pkt, uint16_t *iphc,
			     struct net_6lo_context *dst)
{
	*iphc |= NET_6LO_IPHC_DAC_1;

	if (net_ipv6_addr_based_on_ll((struct in6_addr *)ipv6->dst,
				      net_pkt_lladdr_dst(pkt))) {
		NET_DBG("DAM_11 dst addr fully elided");

		*iphc |= NET_6LO_IPHC_DAM_11;
		return inline_ptr;
	}

	/* Following 64 bits are 0000:00ff:fe00:XXXX */
	if (net_6lo_addr_16_bit_compressible((struct in6_addr *)ipv6->dst)) {
		NET_DBG("DAM_10 dst addr 16 bit compressible");

		*iphc |= NET_6LO_IPHC_DAM_10;
		inline_ptr -= sizeof(uint16_t);
		memmove(inline_ptr, &ipv6->dst[14], sizeof(uint16_t));
		return inline_ptr;
	}

	NET_DBG("DAM_01 remaining 64 bits are inlined");

	/* Remaining 64 bits are in-line */
	*iphc |= NET_6LO_IPHC_DAM_01;

	inline_ptr -= 8U;
	memmove(inline_ptr, &ipv6->dst[8], 8U);

	return inline_ptr;
}
#endif

/* Helper to compress Next header UDP */
static inline uint8_t *compress_nh_udp(struct net_udp_hdr *udp, uint8_t *inline_ptr,
				    bool compress_checksum)
{
	uint8_t nhc = NET_6LO_NHC_UDP_BARE;
	uint8_t *inline_ptr_udp = inline_ptr;
	uint8_t tmp;

	/* 4.3.3 UDP LOWPAN_NHC Format
	 *   0   1   2   3   4   5   6   7
	 * +---+---+---+---+---+---+---+---+
	 * | 1 | 1 | 1 | 1 | 0 | C |   P   |
	 * +---+---+---+---+---+---+---+---+
	 */

	/* Port compression
	 * 00:  All 16 bits for src and dst are inlined.
	 * 01:  All 16 bits for src port inlined. First 8 bits of dst port is
	 *      0xf0 and elided.  The remaining 8 bits of dst port inlined.
	 * 10:  First 8 bits of src port 0xf0 and elided. The remaining 8 bits
	 *      of src port inlined. All 16 bits of dst port inlined.
	 * 11:  First 12 bits of both src and dst are 0xf0b and elided. The
	 *      remaining 4 bits for each are inlined.
	 */

	if (compress_checksum) {
		nhc |= NET_6LO_NHC_UDP_CHECKSUM;
	} else {
		inline_ptr_udp -= sizeof(udp->chksum);
		memmove(inline_ptr_udp, &udp->chksum, sizeof(udp->chksum));
	}

	if ((((htons(udp->src_port) >> 4) & 0xFFF) ==
	    NET_6LO_NHC_UDP_4_BIT_PORT) &&
	    (((htons(udp->dst_port) >> 4) & 0xFFF) ==
	    NET_6LO_NHC_UDP_4_BIT_PORT)) {

		NET_DBG("UDP ports src and dst 4 bits inlined");
		/** src: first 16 bits elided, next 4 bits inlined
		  * dst: first 16 bits elided, next 4 bits inlined
		  */
		nhc |= NET_6LO_NHC_UDP_PORT_11;

		tmp = (uint8_t)(htons(udp->src_port));
		tmp = tmp << 4;

		tmp |= (((uint8_t)(htons(udp->dst_port))) & 0x0F);
		inline_ptr_udp -= sizeof(tmp);
		*inline_ptr_udp = tmp;

	} else if (((htons(udp->dst_port) >> 8) & 0xFF) ==
		   NET_6LO_NHC_UDP_8_BIT_PORT) {

		NET_DBG("UDP ports src full, dst 8 bits inlined");
		/* dst: first 8 bits elided, next 8 bits inlined
		 * src: fully carried inline
		 */
		nhc |= NET_6LO_NHC_UDP_PORT_01;

		inline_ptr_udp -= sizeof(uint8_t);
		*inline_ptr_udp = (uint8_t)(htons(udp->dst_port));

		inline_ptr_udp -= sizeof(udp->src_port);
		memmove(inline_ptr_udp, &udp->src_port, sizeof(udp->src_port));

	} else if (((htons(udp->src_port) >> 8) & 0xFF) ==
		    NET_6LO_NHC_UDP_8_BIT_PORT) {

		NET_DBG("UDP ports src 8bits, dst full inlined");
		/* src: first 8 bits elided, next 8 bits inlined
		 * dst: fully carried inline
		 */
		nhc |= NET_6LO_NHC_UDP_PORT_10;

		inline_ptr_udp -= sizeof(udp->dst_port);
		memmove(inline_ptr_udp, &udp->dst_port, sizeof(udp->dst_port));

		inline_ptr_udp -= sizeof(uint8_t);
		*inline_ptr_udp = (uint8_t)(htons(udp->src_port));

	} else {
		NET_DBG("Can not compress ports, ports are inlined");

		/* can not compress ports, ports are inlined */
		inline_ptr_udp -= sizeof(udp->dst_port) + sizeof(udp->src_port);
		memmove(inline_ptr_udp, &udp->src_port,
			sizeof(udp->dst_port) + sizeof(udp->src_port));
	}

	inline_ptr_udp -= sizeof(nhc);
	*inline_ptr_udp = nhc;

	return inline_ptr_udp;
}

#if defined(CONFIG_NET_6LO_CONTEXT)

static struct net_6lo_context *get_src_addr_ctx(struct net_pkt *pkt,
						struct net_ipv6_hdr *ipv6)
{
	/* If compress flag is unset means use only in uncompression. */
	struct net_6lo_context *src;

	src = get_6lo_context_by_addr(net_pkt_iface(pkt),
				      (struct in6_addr *)ipv6->src);
	if (!src || !src->compress) {
		return NULL;
	}

	return src;
}

static struct net_6lo_context *get_dst_addr_ctx(struct net_pkt *pkt,
						struct net_ipv6_hdr *ipv6)
{
	/* If compress flag is unset means use only in uncompression. */
	struct net_6lo_context *dst;

	dst = get_6lo_context_by_addr(net_pkt_iface(pkt),
				      (struct in6_addr *)ipv6->dst);
	if (!dst || !dst->compress) {
		return NULL;
	}

	return dst;
}
#endif /* CONFIG_NET_6LO_CONTEXT */

/* RFC 6282 LOWPAN IPHC Encoding format (3.1)
 *  Base Format
 *   0                                       1
 *   0   1   2   3   4   5   6   7   8   9   0   1   2   3   4   5
 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 * | 0 | 1 | 1 |  TF   |NH | HLIM  |CID|SAC|  SAM  | M |DAC|  DAM  |
 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 */
static inline int compress_IPHC_header(struct net_pkt *pkt)
{
#if defined(CONFIG_NET_6LO_CONTEXT)
	struct net_6lo_context *src_ctx = NULL;
	struct net_6lo_context *dst_ctx = NULL;
#endif
	uint8_t compressed = 0;
	uint16_t iphc = (NET_6LO_DISPATCH_IPHC << 8);
	struct net_ipv6_hdr *ipv6 = NET_IPV6_HDR(pkt);
	struct net_udp_hdr *udp;
	uint8_t *inline_pos;

	if (pkt->frags->len < NET_IPV6H_LEN) {
		NET_ERR("Invalid length %d, min %d",
			pkt->frags->len, NET_IPV6H_LEN);
		return -EINVAL;
	}

	if (ipv6->nexthdr == IPPROTO_UDP &&
	    pkt->frags->len < NET_IPV6UDPH_LEN) {
		NET_ERR("Invalid length %d, min %d",
			pkt->frags->len, NET_IPV6UDPH_LEN);
		return -EINVAL;
	}

	inline_pos = pkt->buffer->data + NET_IPV6H_LEN;

	if (ipv6->nexthdr == IPPROTO_UDP) {
		udp = (struct net_udp_hdr *)inline_pos;
		inline_pos += NET_UDPH_LEN;

		inline_pos = compress_nh_udp(udp, inline_pos, false);
	}

	if (net_6lo_ll_prefix_padded_with_zeros((struct in6_addr *)ipv6->dst)) {
		inline_pos = compress_da(ipv6, pkt, inline_pos, &iphc);
		goto da_end;
	}

	if (net_ipv6_is_addr_mcast((struct in6_addr *)ipv6->dst)) {
		inline_pos = compress_da_mcast(ipv6, inline_pos, &iphc);
		goto da_end;
	}

#if defined(CONFIG_NET_6LO_CONTEXT)
	dst_ctx = get_dst_addr_ctx(pkt, ipv6);
	if (dst_ctx) {
		iphc |= NET_6LO_IPHC_CID_1;
		inline_pos = compress_da_ctx(ipv6, inline_pos, pkt, &iphc,
					     dst_ctx);
		goto da_end;
	}
#endif
	inline_pos = set_da_inline(ipv6, inline_pos, &iphc);
da_end:

	if (net_6lo_ll_prefix_padded_with_zeros((struct in6_addr *)ipv6->src)) {
		inline_pos = compress_sa(ipv6, pkt, inline_pos, &iphc);
		goto sa_end;
	}

	if (net_ipv6_is_addr_unspecified((struct in6_addr *)ipv6->src)) {
		NET_DBG("SAM_00, SAC_1 unspecified src address");

		/* Unspecified IPv6 src address */
		iphc |= NET_6LO_IPHC_SAC_1;
		iphc |= NET_6LO_IPHC_SAM_00;
		goto sa_end;
	}

#if defined(CONFIG_NET_6LO_CONTEXT)
	src_ctx = get_src_addr_ctx(pkt, ipv6);
	if (src_ctx) {
		inline_pos = compress_sa_ctx(ipv6, inline_pos, pkt, &iphc,
					     src_ctx);
		iphc |= NET_6LO_IPHC_CID_1;
		goto sa_end;
	}
#endif
	inline_pos = set_sa_inline(ipv6, inline_pos, &iphc);
sa_end:

	inline_pos = compress_hoplimit(ipv6, inline_pos, &iphc);
	inline_pos = compress_nh(ipv6, inline_pos, &iphc);
	inline_pos = compress_tfl(ipv6, inline_pos, &iphc);

#if defined(CONFIG_NET_6LO_CONTEXT)
	if (iphc & NET_6LO_IPHC_CID_1) {
		inline_pos -= sizeof(uint8_t);
		*inline_pos = 0;

		if (src_ctx) {
			*inline_pos = src_ctx->cid << 4;
		}

		if (dst_ctx) {
			*inline_pos |= dst_ctx->cid & 0x0F;
		}
	}
#endif

	inline_pos -= sizeof(iphc);
	iphc = htons(iphc);
	memmove(inline_pos, &iphc, sizeof(iphc));

	compressed = inline_pos - pkt->buffer->data;

	net_buf_pull(pkt->buffer, compressed);

	return compressed;
}

/* Helper to uncompress Traffic class and Flow label */
static inline uint8_t *uncompress_tfl(uint16_t iphc, uint8_t *cursor,
				  struct net_ipv6_hdr *ipv6)
{
	uint8_t tcl;

	/* Uncompress tcl and flow label */
	switch (iphc & NET_6LO_IPHC_TF_11) {
	case NET_6LO_IPHC_TF_00:
		NET_DBG("ECN + DSCP + 4-bit Pad + Flow Label");

		tcl = *cursor;
		cursor++;
		tcl = (tcl >> 6) | (tcl << 2);

		ipv6->vtc |= ((tcl & 0xF0) >> 4);
		ipv6->tcflow = ((tcl & 0x0F) << 4) | (*cursor & 0x0F);
		cursor++;

		memmove(&ipv6->flow, cursor, sizeof(ipv6->flow));
		cursor += sizeof(ipv6->flow);
		break;
	case NET_6LO_IPHC_TF_01:
		NET_DBG("ECN + 2-bit Pad + Flow Label, DSCP is elided");

		tcl = ((*cursor & 0xF0) >> 6);
		ipv6->tcflow = ((tcl & 0x0F) << 4) | (*cursor & 0x0F);
		cursor++;

		memmove(&ipv6->flow, cursor, sizeof(ipv6->flow));
		cursor += sizeof(ipv6->flow);

		break;
	case NET_6LO_IPHC_TF_10:
		NET_DBG("Flow label elided");

		tcl = *cursor;
		cursor++;
		tcl = (tcl >> 6) | (tcl << 2);

		ipv6->vtc |= ((tcl & 0xF0) >> 4);
		ipv6->tcflow = (tcl & 0x0F) << 4;
		ipv6->flow = 0U;

		break;
	case NET_6LO_IPHC_TF_11:
		NET_DBG("Tcl and Flow label elided");

		ipv6->tcflow = 0U;
		ipv6->flow = 0U;

		break;
	}

	return cursor;
}

/* Helper to uncompress Hoplimit */
static inline uint8_t *uncompress_hoplimit(uint16_t iphc, uint8_t *cursor,
				       struct net_ipv6_hdr *ipv6)
{
	switch (iphc & NET_6LO_IPHC_HLIM_MASK) {
	case NET_6LO_IPHC_HLIM:
		ipv6->hop_limit = *cursor;
		cursor++;

		break;
	case NET_6LO_IPHC_HLIM1:
		ipv6->hop_limit = 1U;

		break;
	case NET_6LO_IPHC_HLIM64:
		ipv6->hop_limit = 64U;

		break;
	case NET_6LO_IPHC_HLIM255:
		ipv6->hop_limit = 255U;

		break;
	}

	return cursor;
}

/* Helper to uncompress Source Address */
static inline uint8_t *uncompress_sa(uint16_t iphc, uint8_t *cursor,
				 struct net_ipv6_hdr *ipv6,
				 struct net_pkt *pkt)
{
	struct in6_addr src_ip;

	NET_DBG("SAC_0");

	net_ipv6_addr_copy_raw((uint8_t *)&src_ip, ipv6->src);

	switch (iphc & NET_6LO_IPHC_SAM_MASK) {
	case NET_6LO_IPHC_SAM_00:
		NET_DBG("SAM_00 full src addr inlined");

		memmove(src_ip.s6_addr, cursor, sizeof(src_ip.s6_addr));
		cursor += sizeof(src_ip.s6_addr);

		break;
	case NET_6LO_IPHC_SAM_01:
		NET_DBG("SAM_01 last 64 bits are inlined");

		memmove(&src_ip.s6_addr[8], cursor, 8);
		cursor += 8U;

		src_ip.s6_addr32[0] = 0x00;
		src_ip.s6_addr32[1] = 0x00;
		src_ip.s6_addr[0] = 0xFE;
		src_ip.s6_addr[1] = 0x80;

		break;
	case NET_6LO_IPHC_SAM_10:
		NET_DBG("SAM_10 src addr 16 bit compressed");

		memmove(&src_ip.s6_addr[14], cursor, 2);
		cursor += 2U;
		src_ip.s6_addr16[6] = 0x00;

		src_ip.s6_addr32[0] = 0x00;
		src_ip.s6_addr32[1] = 0x00;
		src_ip.s6_addr32[2] = 0x00;
		src_ip.s6_addr[0] = 0xFE;
		src_ip.s6_addr[1] = 0x80;
		src_ip.s6_addr[11] = 0xFF;
		src_ip.s6_addr[12] = 0xFE;

		break;
	case NET_6LO_IPHC_SAM_11:
		NET_DBG("SAM_11 generate src addr from ll");

		net_ipv6_addr_create_iid(&src_ip, net_pkt_lladdr_src(pkt));

		break;
	}

	net_ipv6_addr_copy_raw(ipv6->src, (uint8_t *)&src_ip);

	return cursor;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
static inline uint8_t *uncompress_sa_ctx(uint16_t iphc, uint8_t *cursor,
				     struct net_ipv6_hdr *ipv6,
				     struct net_6lo_context *ctx,
				     struct net_pkt *pkt)
{
	struct in6_addr src_ip;

	net_ipv6_addr_copy_raw((uint8_t *)&src_ip, ipv6->src);

	switch (iphc & NET_6LO_IPHC_SAM_MASK) {
	case NET_6LO_IPHC_SAM_01:
		NET_DBG("SAM_01 last 64 bits are inlined");

		/* First 8 bytes are from context */
		memmove(&src_ip.s6_addr[0], &ctx->prefix.s6_addr[0], 8);

		/* And the rest are carried in-line*/
		memmove(&src_ip.s6_addr[8], cursor, 8);
		cursor += 8U;

		break;
	case NET_6LO_IPHC_SAM_10:
		NET_DBG("SAM_10 src addr 16 bit compressed");

		/* 16 bit carried in-line */
		memmove(&src_ip.s6_addr[14], cursor, 2);
		cursor += 2U;

		/* First 8 bytes are from context */
		memmove(&src_ip.s6_addr[0], &ctx->prefix.s6_addr[0], 8);

		src_ip.s6_addr32[2] = 0x00;
		src_ip.s6_addr16[6] = 0x00;
		src_ip.s6_addr[11] = 0xFF;
		src_ip.s6_addr[12] = 0xFE;

		break;
	case NET_6LO_IPHC_SAM_11:
		NET_DBG("SAM_11 generate src addr from ll");

		/* RFC 6282, 3.1.1. If SAC = 1 and SAM = 11
		 * Derive addr using context information and
		 * the encapsulating header.
		 * (e.g., 802.15.4 or IPv6 source address).
		 */
		net_ipv6_addr_create_iid(&src_ip, net_pkt_lladdr_src(pkt));

		/* net_ipv6_addr_create_iid will copy first 8 bytes
		 * as link local prefix.
		 * Overwrite first 8 bytes from context prefix here.
		 */
		memmove(&src_ip.s6_addr[0], &ctx->prefix.s6_addr[0], 8);
		break;
	}

	net_ipv6_addr_copy_raw(ipv6->src, (uint8_t *)&src_ip);

	return cursor;
}
#endif

/* Helpers to uncompress Destination Address */
static inline uint8_t *uncompress_da_mcast(uint16_t iphc, uint8_t *cursor,
				       struct net_ipv6_hdr *ipv6)
{
	struct in6_addr dst_ip;

	NET_DBG("Dst is multicast");

	net_ipv6_addr_copy_raw((uint8_t *)&dst_ip, ipv6->dst);

	if (iphc & NET_6LO_IPHC_DAC_1) {
		NET_WARN("Unsupported DAM options");
		return 0;
	}

	/* If M=1 and DAC=0:
	 * 00: 128 bits, The full address is carried in-line.
	 * 01:  48 bits, The address takes the form ffXX::00XX:XXXX:XXXX.
	 * 10:  32 bits, The address takes the form ffXX::00XX:XXXX.
	 * 11:   8 bits, The address takes the form ff02::00XX.
	 */

	switch (iphc & NET_6LO_IPHC_DAM_MASK) {
	case NET_6LO_IPHC_DAM_00:
		NET_DBG("DAM_00 full dst addr inlined");

		memmove(&dst_ip.s6_addr[0], cursor,
			sizeof(dst_ip.s6_addr));

		cursor += sizeof(dst_ip.s6_addr);
		break;
	case NET_6LO_IPHC_DAM_01:
		NET_DBG("DAM_01 2nd byte and last five bytes");

		dst_ip.s6_addr[1] = *cursor;
		cursor++;
		memmove(&dst_ip.s6_addr[11], cursor, 5);
		cursor += 5U;


		dst_ip.s6_addr[0] = 0xFF;
		dst_ip.s6_addr16[1] = 0x00;
		dst_ip.s6_addr32[1] = 0x00;
		dst_ip.s6_addr[10] = 0x00;
		dst_ip.s6_addr16[4] = 0x00;

		break;
	case NET_6LO_IPHC_DAM_10:
		NET_DBG("DAM_10 2nd byte and last three bytes");

		dst_ip.s6_addr[1] = *cursor;
		cursor++;
		memmove(&dst_ip.s6_addr[13], cursor, 3);
		cursor += 3U;

		dst_ip.s6_addr[0] = 0xFF;
		dst_ip.s6_addr16[1] = 0x00;
		dst_ip.s6_addr32[1] = 0x00;
		dst_ip.s6_addr32[2] = 0x00;
		dst_ip.s6_addr[12] = 0x00;

		break;
	case NET_6LO_IPHC_DAM_11:
		NET_DBG("DAM_11 8 bit compressed");

		dst_ip.s6_addr[15] = *cursor;
		cursor++;
		dst_ip.s6_addr[14] = 0x00;

		dst_ip.s6_addr32[0] = 0x00;
		dst_ip.s6_addr32[1] = 0x00;
		dst_ip.s6_addr32[2] = 0x00;
		dst_ip.s6_addr16[6] = 0x00;
		dst_ip.s6_addr[0] = 0xFF;
		dst_ip.s6_addr[1] = 0x02;

		break;
	}

	net_ipv6_addr_copy_raw(ipv6->dst, (uint8_t *)&dst_ip);

	return cursor;
}

/* Helper to uncompress Destination Address */
static inline uint8_t *uncompress_da(uint16_t iphc, uint8_t *cursor,
				 struct net_ipv6_hdr *ipv6,
				 struct net_pkt *pkt)
{
	struct in6_addr dst_ip;

	NET_DBG("DAC_0");

	net_ipv6_addr_copy_raw((uint8_t *)&dst_ip, ipv6->dst);

	switch (iphc & NET_6LO_IPHC_DAM_MASK) {
	case NET_6LO_IPHC_DAM_00:
		NET_DBG("DAM_00 full dst addr inlined");

		memmove(&dst_ip.s6_addr[0], cursor,
			sizeof(dst_ip.s6_addr));
		cursor += sizeof(dst_ip.s6_addr);

		break;
	case NET_6LO_IPHC_DAM_01:
		NET_DBG("DAM_01 last 64 bits are inlined");

		memmove(&dst_ip.s6_addr[8], cursor, 8);
		cursor += 8U;

		dst_ip.s6_addr32[0] = 0x00;
		dst_ip.s6_addr32[1] = 0x00;
		dst_ip.s6_addr[0] = 0xFE;
		dst_ip.s6_addr[1] = 0x80;

		break;
	case NET_6LO_IPHC_DAM_10:
		NET_DBG("DAM_10 dst addr 16 bit compressed");

		memmove(&dst_ip.s6_addr[14], cursor, 2);
		cursor += 2U;

		dst_ip.s6_addr32[0] = 0x00;
		dst_ip.s6_addr32[1] = 0x00;
		dst_ip.s6_addr32[2] = 0x00;
		dst_ip.s6_addr16[6] = 0x00;
		dst_ip.s6_addr[0] = 0xFE;
		dst_ip.s6_addr[1] = 0x80;
		dst_ip.s6_addr[11] = 0xFF;
		dst_ip.s6_addr[12] = 0xFE;

		break;
	case NET_6LO_IPHC_DAM_11:
		NET_DBG("DAM_11 generate dst addr from ll");

		net_ipv6_addr_create_iid(&dst_ip, net_pkt_lladdr_dst(pkt));

		break;
	}

	net_ipv6_addr_copy_raw(ipv6->dst, (uint8_t *)&dst_ip);

	return cursor;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
static inline uint8_t *uncompress_da_ctx(uint16_t iphc, uint8_t *cursor,
				     struct net_ipv6_hdr *ipv6,
				     struct net_6lo_context *ctx,
				     struct net_pkt *pkt)
{
	struct in6_addr dst_ip;

	NET_DBG("DAC_1");

	net_ipv6_addr_copy_raw((uint8_t *)&dst_ip, ipv6->dst);

	switch (iphc & NET_6LO_IPHC_DAM_MASK) {
	case NET_6LO_IPHC_DAM_01:
		NET_DBG("DAM_01 last 64 bits are inlined");

		/* Last 8 bytes carried in-line */
		memmove(&dst_ip.s6_addr[8], cursor, 8);
		cursor += 8U;

		/* First 8 bytes are from context */
		memmove(&dst_ip.s6_addr[0], &ctx->prefix.s6_addr[0], 8);

		break;
	case NET_6LO_IPHC_DAM_10:
		NET_DBG("DAM_10 src addr 16 bit compressed");

		/* 16 bit carried in-line */
		memmove(&dst_ip.s6_addr[14], cursor, 2);
		cursor += 2U;

		/* First 8 bytes are from context */
		memmove(&dst_ip.s6_addr[0], &ctx->prefix.s6_addr[0], 8);

		dst_ip.s6_addr32[2] = 0x00;
		dst_ip.s6_addr16[6] = 0x00;
		dst_ip.s6_addr[11] = 0xFF;
		dst_ip.s6_addr[12] = 0xFE;

		break;
	case NET_6LO_IPHC_DAM_11:
		NET_DBG("DAM_11 generate src addr from ll");

		/* RFC 6282, 3.1.1. If SAC = 1 and SAM = 11
		 * Derive addr using context information and
		 * the encapsulating header.
		 * (e.g., 802.15.4 or IPv6 source address).
		 */
		net_ipv6_addr_create_iid(&dst_ip, net_pkt_lladdr_dst(pkt));

		/* net_ipv6_addr_create_iid will copy first 8 bytes
		 * as link local prefix.
		 * Overwrite first 8 bytes from context prefix here.
		 */
		memmove(&dst_ip.s6_addr[0], &ctx->prefix.s6_addr[0], 8);

		break;
	}

	net_ipv6_addr_copy_raw(ipv6->dst, (uint8_t *)&dst_ip);

	return cursor;
}
#endif

/* Helper to uncompress NH UDP */
static uint8_t *uncompress_nh_udp(uint8_t nhc, uint8_t *cursor,
				      struct net_udp_hdr *udp)
{

	/* Port uncompression
	 * 00:  All 16 bits for src and dst are inlined
	 * 01: src, 16 bits are lined, dst(0xf0) 8 bits are inlined
	 * 10: dst, 16 bits are lined, src(0xf0) 8 bits are inlined
	 * 11: src, dst (0xf0b) 4 bits are inlined
	 */

	/* UDP header uncompression */
	switch (nhc & NET_6LO_NHC_UDP_PORT_11) {
	case NET_6LO_NHC_UDP_PORT_00:
		NET_DBG("src and dst ports are inlined");

		memmove(&udp->src_port, cursor, sizeof(udp->src_port));
		cursor += sizeof(udp->src_port);
		memmove(&udp->dst_port, cursor, sizeof(udp->dst_port));
		cursor += sizeof(udp->dst_port);

		break;
	case NET_6LO_NHC_UDP_PORT_01:
		NET_DBG("src full, dst 8 bits inlined");

		memmove(&udp->src_port, cursor, sizeof(udp->src_port));
		cursor += sizeof(udp->src_port);
		udp->dst_port = htons(((uint16_t)NET_6LO_NHC_UDP_8_BIT_PORT << 8) |
				*cursor);
		cursor++;

		break;
	case NET_6LO_NHC_UDP_PORT_10:
		NET_DBG("src 8 bits, dst full inlined");

		udp->src_port = htons(((uint16_t)NET_6LO_NHC_UDP_8_BIT_PORT << 8) |
				*cursor);
		cursor++;
		memmove(&udp->dst_port, cursor, sizeof(udp->dst_port));
		cursor += sizeof(udp->dst_port);

		break;
	case NET_6LO_NHC_UDP_PORT_11:
		NET_DBG("src and dst 4 bits inlined");

		udp->src_port = htons((NET_6LO_NHC_UDP_4_BIT_PORT << 4) |
				(*cursor >> 4));

		udp->dst_port = htons((NET_6LO_NHC_UDP_4_BIT_PORT << 4) |
				(*cursor & 0x0F));
		cursor++;

		break;
	}

	if (!(nhc & NET_6LO_NHC_UDP_CHECKSUM)) {
		memmove(&udp->chksum, cursor, sizeof(udp->chksum));
		cursor += sizeof(udp->chksum);
	}

	return cursor;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
/* Helper function to uncompress src and dst contexts */
static inline void uncompress_cid(struct net_pkt *pkt, uint8_t cid,
				  struct net_6lo_context **src,
				  struct net_6lo_context **dst)
{
	uint8_t cid_tmp;

	/* Extract source and destination Context Index,
	 * Either src or dest address is context based or both.
	 */
	cid_tmp = (cid >> 4) & 0x0F;
	*src = get_6lo_context_by_cid(net_pkt_iface(pkt), cid_tmp);
	if (!(*src)) {
		NET_DBG("Unknown src cid %d", cid_tmp);
	}

	cid_tmp = cid & 0x0F;
	*dst = get_6lo_context_by_cid(net_pkt_iface(pkt), cid_tmp);
	if (!(*dst)) {
		NET_DBG("Unknown dst cid %d", cid_tmp);
	}
}
#endif

static bool uncompress_IPHC_header(struct net_pkt *pkt)
{
	struct net_udp_hdr *udp = NULL;
	struct net_buf *frag = NULL;
	uint8_t nhc = 0;
	int nhc_inline_size = 0;
	struct net_ipv6_hdr *ipv6;
	uint16_t len;
	uint16_t iphc;
	int inline_size, compressed_hdr_size;
	size_t diff;
	uint8_t *cursor;
#if defined(CONFIG_NET_6LO_CONTEXT)
	struct net_6lo_context *src = NULL;
	struct net_6lo_context *dst = NULL;
#endif

	iphc = ntohs(UNALIGNED_GET((uint16_t *)pkt->buffer->data));

	inline_size = get_ihpc_inlined_size(iphc);
	if (inline_size < 0) {
		return false;
	}

	compressed_hdr_size = sizeof(iphc) + inline_size;
	diff = sizeof(struct net_ipv6_hdr) - compressed_hdr_size;

	if (iphc & NET_6LO_IPHC_NH_MASK) {
		nhc = *(pkt->buffer->data + sizeof(iphc) + inline_size);
		if ((nhc & 0xF8) != NET_6LO_NHC_UDP_BARE) {
			NET_ERR("Unsupported next header");
			return false;
		}

		nhc_inline_size = get_udp_nhc_inlined_size(nhc);
		compressed_hdr_size += sizeof(uint8_t) + nhc_inline_size;
		diff += sizeof(struct net_udp_hdr) - sizeof(uint8_t) -
			nhc_inline_size;
	}

	if (pkt->buffer->len < compressed_hdr_size) {
		NET_ERR("Scattered compressed header?");
		return false;
	}

	if (net_buf_tailroom(pkt->buffer) >= diff) {
		NET_DBG("Enough tailroom. Uncompress inplace");
		frag = pkt->buffer;
		net_buf_add(frag, diff);
		cursor = frag->data + diff;
		memmove(cursor, frag->data, frag->len - diff);
	} else {
		NET_DBG("Not enough tailroom. Get new fragment");
		cursor =  pkt->buffer->data;
		frag = net_pkt_get_frag(pkt, NET_6LO_RX_PKT_TIMEOUT);
		if (!frag) {
			NET_ERR("Can't get frag for uncompression");
			return false;
		}

		net_buf_pull(pkt->buffer, compressed_hdr_size);
		net_buf_add(frag, nhc ? NET_IPV6UDPH_LEN : NET_IPV6H_LEN);
	}

	ipv6 = (struct net_ipv6_hdr *)(frag->data);
	cursor += sizeof(iphc);

	if (iphc & NET_6LO_IPHC_CID_1) {
#if defined(CONFIG_NET_6LO_CONTEXT)
		uncompress_cid(pkt, *cursor, &src, &dst);
		cursor++;
#else
		NET_ERR("Context based uncompression not enabled");
		return false;
#endif
	}

	/* Version is always 6 */
	ipv6->vtc = 0x60;
	net_pkt_set_ip_hdr_len(pkt, NET_IPV6H_LEN);

	/* Uncompress Traffic class and Flow label */
	cursor = uncompress_tfl(iphc, cursor, ipv6);

	if (!(iphc & NET_6LO_IPHC_NH_MASK)) {
		ipv6->nexthdr = *cursor;
		cursor++;
	}

	/* Uncompress Hoplimit */
	cursor = uncompress_hoplimit(iphc, cursor, ipv6);

	/* Uncompress Source Address */
	if (iphc & NET_6LO_IPHC_SAC_1) {
		NET_DBG("SAC_1");

		if ((iphc & NET_6LO_IPHC_SAM_MASK) == NET_6LO_IPHC_SAM_00) {
			NET_DBG("SAM_00 unspecified address");
			memset(&ipv6->src[0], 0,
				sizeof(ipv6->src));
		} else if (IS_ENABLED(CONFIG_NET_6LO_CONTEXT)) {
#if defined(CONFIG_NET_6LO_CONTEXT)
			if (!src) {
				NET_ERR("Src context doesn't exists");
				goto fail;
			}

			cursor = uncompress_sa_ctx(iphc, cursor, ipv6, src, pkt);
#endif
		} else {
			NET_ERR("Context based uncompression not enabled");
			goto fail;
		}
	} else {
		cursor = uncompress_sa(iphc, cursor, ipv6, pkt);
	}

	/* Uncompress Destination Address */
	if (iphc & NET_6LO_IPHC_M_1) {
		if (iphc & NET_6LO_IPHC_DAC_1) {
			/* TODO: DAM00 Unicast-Prefix-based IPv6 Multicast
			 * Addresses. DAM_01, DAM_10 and DAM_11 are reserved.
			 */
			NET_ERR("DAC_1 and M_1 is not supported");
			goto fail;
		} else {
			cursor = uncompress_da_mcast(iphc, cursor, ipv6);
		}
	} else {
		if (iphc & NET_6LO_IPHC_DAC_1) {
#if defined(CONFIG_NET_6LO_CONTEXT)
			if (!dst) {
				NET_ERR("Dst context doesn't exists");
				goto fail;
			}

			cursor = uncompress_da_ctx(iphc, cursor, ipv6, dst, pkt);
#else
			NET_ERR("Context based uncompression not enabled");
			goto fail;
#endif
		} else {
			cursor = uncompress_da(iphc, cursor, ipv6, pkt);
		}
	}

	if (iphc & NET_6LO_IPHC_NH_MASK) {
		ipv6->nexthdr = IPPROTO_UDP;
		udp = (struct net_udp_hdr *)(frag->data + NET_IPV6H_LEN);
		/* skip nhc */
		cursor++;
		cursor = uncompress_nh_udp(nhc, cursor, udp);
	}

	if (frag != pkt->buffer) {
		/* Insert the fragment (this one holds uncompressed headers) */
		net_pkt_frag_insert(pkt, frag);
	}

	/* Set IPv6 header and UDP (if next header is) length */
	len = net_pkt_get_len(pkt) - NET_IPV6H_LEN;
	ipv6->len = htons(len);

	if (ipv6->nexthdr == IPPROTO_UDP && udp) {
		udp->len = htons(len);

		if (nhc & NET_6LO_NHC_UDP_CHECKSUM) {
			udp->chksum = net_calc_chksum_udp(pkt);
		}
	}

	net_pkt_cursor_init(pkt);

	return true;

fail:
	if (frag != pkt->buffer) {
		net_pkt_frag_unref(frag);
	}

	return false;
}

/* Adds IPv6 dispatch as first byte and adjust fragments  */
static inline int compress_ipv6_header(struct net_pkt *pkt)
{
	struct net_buf *buffer = pkt->buffer;

	if (net_buf_tailroom(buffer) >= 1U) {
		memmove(buffer->data + 1U, buffer->data, buffer->len);
		net_buf_add(buffer, 1U);
		buffer->data[0] = NET_6LO_DISPATCH_IPV6;
		return 0;
	}

	buffer = net_pkt_get_frag(pkt, K_FOREVER);
	if (!buffer) {
		return -ENOBUFS;
	}

	buffer->data[0] = NET_6LO_DISPATCH_IPV6;
	net_buf_add(buffer, 1);

	net_pkt_frag_insert(pkt, buffer);

	/* Compact the fragments, so that gaps will be filled */
	net_pkt_compact(pkt);

	return 0;
}

static inline bool uncompress_ipv6_header(struct net_pkt *pkt)
{
	/* Pull off IPv6 dispatch header and adjust data and length */
	net_buf_pull(pkt->buffer, 1U);
	net_pkt_cursor_init(pkt);

	return true;
}

int net_6lo_compress(struct net_pkt *pkt, bool iphc)
{
	if (iphc) {
		return compress_IPHC_header(pkt);
	} else {
		return compress_ipv6_header(pkt);
	}
}

bool net_6lo_uncompress(struct net_pkt *pkt)
{
	NET_ASSERT(pkt && pkt->frags);

	if ((pkt->frags->data[0] & NET_6LO_DISPATCH_IPHC_MASK) ==
	    NET_6LO_DISPATCH_IPHC) {
		/* Uncompress IPHC header */
		return uncompress_IPHC_header(pkt);

	} else if (pkt->frags->data[0] == NET_6LO_DISPATCH_IPV6) {
		/* Uncompress IPv6 header, it has only IPv6 dispatch in the
		 * beginning */
		return uncompress_ipv6_header(pkt);
	}

	NET_DBG("pkt %p is not compressed", pkt);

	return true;
}

int net_6lo_uncompress_hdr_diff(struct net_pkt *pkt)
{
	int inline_size, compressed_hdr_size, nhc_inline_size, diff;
	uint16_t iphc;
	uint8_t nhc;

	if (pkt->frags->data[0] == NET_6LO_DISPATCH_IPV6) {
		return -1;
	}

	if ((pkt->frags->data[0] & NET_6LO_DISPATCH_IPHC_MASK) !=
	    NET_6LO_DISPATCH_IPHC) {
		return 0;
	}

	iphc = ntohs(UNALIGNED_GET((uint16_t *)pkt->buffer->data));

	inline_size = get_ihpc_inlined_size(iphc);
	if (inline_size < 0) {
		return INT_MAX;
	}

	compressed_hdr_size = sizeof(iphc) + inline_size;
	diff = sizeof(struct net_ipv6_hdr) - compressed_hdr_size;

	if (iphc & NET_6LO_IPHC_NH_MASK) {
		nhc = *(pkt->buffer->data + sizeof(iphc) + inline_size);
		if ((nhc & 0xF8) != NET_6LO_NHC_UDP_BARE) {
			NET_ERR("Unsupported next header");
			return INT_MAX;
		}

		nhc_inline_size = get_udp_nhc_inlined_size(nhc);
		diff += sizeof(struct net_udp_hdr) - sizeof(uint8_t) -
			nhc_inline_size;
	}

	return diff;
}
