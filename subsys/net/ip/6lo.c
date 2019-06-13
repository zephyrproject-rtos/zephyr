/** @file
 * @brief 6lopan related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_6lo, CONFIG_NET_6LO_LOG_LEVEL);

#include <errno.h>
#include <net/net_core.h>
#include <net/net_if.h>
#include <net/net_stats.h>
#include <net/udp.h>

#include "net_private.h"
#include "6lo.h"
#include "6lo_private.h"

#if defined(CONFIG_NET_6LO_CONTEXT)
struct net_6lo_context {
	struct in6_addr prefix;
	struct net_if *iface;
	u16_t lifetime;
	u8_t is_used		: 1;
	u8_t compress	: 1;
	u8_t cid		: 4;
	u8_t unused		: 2;
} __packed;

static inline u8_t get_6co_compress(struct net_icmpv6_nd_opt_6co *opt)
{
	return (opt->flag & 0x10) >> 4;
}

static inline u8_t get_6co_cid(struct net_icmpv6_nd_opt_6co *opt)
{
	return opt->flag & 0x0F;
}

static struct net_6lo_context ctx_6co[CONFIG_NET_MAX_6LO_CONTEXTS];
#endif

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
static inline void set_6lo_context(struct net_if *iface, u8_t index,
				   struct net_icmpv6_nd_opt_6co *context)

{
	ctx_6co[index].is_used = true;
	ctx_6co[index].iface = iface;

	/*TODO: Start timer */
	ctx_6co[index].lifetime = context->lifetime;
	ctx_6co[index].compress = get_6co_compress(context);
	ctx_6co[index].cid = get_6co_cid(context);

	net_ipaddr_copy(&ctx_6co[index].prefix, &context->prefix);
}

void net_6lo_set_context(struct net_if *iface,
			 struct net_icmpv6_nd_opt_6co *context)
{
	int unused = -1;
	u8_t i;

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
get_6lo_context_by_cid(struct net_if *iface, u8_t cid)
{
	u8_t i;

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
	u8_t i;

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
static u8_t *compress_tfl(struct net_ipv6_hdr *ipv6, u8_t *inline_ptr,
			  u8_t *iphc)
{
	u8_t tcl;

	tcl = ((ipv6->vtc & 0x0F) << 4) | ((ipv6->tcflow & 0xF0) >> 4);
	tcl = (tcl << 6) | (tcl >> 2);   /* ECN(2), DSCP(6) */

	if (((ipv6->tcflow & 0x0F) == 0U) && (ipv6->flow == 0U)) {
		if (((ipv6->vtc & 0x0F) == 0U) && ((ipv6->tcflow & 0xF0) == 0U)) {
			NET_DBG("Traffic class and Flow label elided");

			/* Traffic class and Flow label elided */
			iphc[0] |= NET_6LO_IPHC_TF_11;
		} else {
			NET_DBG("Flow label elided");

			/* Flow label elided */
			iphc[0] |= NET_6LO_IPHC_TF_10;

			inline_ptr -= sizeof(tcl);
			*inline_ptr = tcl;
		}
	} else {
		if (((ipv6->vtc & 0x0F) == 0U) && (ipv6->tcflow & 0x30)) {
			NET_DBG("ECN + 2-bit Pad + Flow Label, DSCP is elided");

			/* ECN + 2-bit Pad + Flow Label, DSCP is elided.*/
			iphc[0] |= NET_6LO_IPHC_TF_01;

			inline_ptr -= sizeof(ipv6->flow);
			memmove(inline_ptr, &ipv6->flow, sizeof(ipv6->flow));

			inline_ptr -= sizeof(u8_t);
			*inline_ptr = (tcl & 0xC0) | (ipv6->tcflow & 0x0F);
		} else {
			NET_DBG("ECN + DSCP + 4-bit Pad + Flow Label");

			/* ECN + DSCP + 4-bit Pad + Flow Label */
			iphc[0] |= NET_6LO_IPHC_TF_00;

			inline_ptr -= sizeof(ipv6->flow);
			memmove(inline_ptr, &ipv6->flow, sizeof(ipv6->flow));

			inline_ptr -= sizeof(u8_t);
			*inline_ptr = ipv6->tcflow & 0x0F;
			inline_ptr -= sizeof(tcl);
			*inline_ptr = tcl;
		}
	}

	return inline_ptr;
}

/* Helper to compress Hop limit */
static u8_t *compress_hoplimit(struct net_ipv6_hdr *ipv6, u8_t *inline_ptr,
			       u8_t *iphc)
{
	/* Hop Limit */
	switch (ipv6->hop_limit) {
	case 1:
		NET_DBG("HLIM compressed (1)");
		iphc[0] |= NET_6LO_IPHC_HLIM1;
		break;
	case 64:
		NET_DBG("HLIM compressed (64)");
		iphc[0] |= NET_6LO_IPHC_HLIM64;
		break;
	case 255:
		NET_DBG("HLIM compressed (255)");
		iphc[0] |= NET_6LO_IPHC_HLIM255;
		break;
	default:
		inline_ptr -= sizeof(ipv6->hop_limit);
		*inline_ptr = ipv6->hop_limit;
		break;
	}

	return inline_ptr;
}

/* Helper to compress Next header */
static u8_t *compress_nh(struct net_ipv6_hdr *ipv6, u8_t *inline_ptr,
			 u8_t *iphc)
{
	/* Next header */
	if (ipv6->nexthdr == IPPROTO_UDP) {
		iphc[0] |= NET_6LO_IPHC_NH_1;
	} else {
		inline_ptr -= sizeof(ipv6->nexthdr);
		*inline_ptr = ipv6->nexthdr;
	}

	return inline_ptr;
}

/* Helpers to compress Source Address */
static u8_t *compress_sa(struct net_ipv6_hdr *ipv6, struct net_pkt *pkt,
			 u8_t *inline_ptr, u8_t *iphc)
{
	NET_ASSERT(net_pkt_lladdr_src(pkt)->addr);

	/* Address is fully elided */
	if (net_ipv6_addr_based_on_ll(&ipv6->src, net_pkt_lladdr_src(pkt))) {
		NET_DBG("SAM_11 src address is fully elided");

		iphc[1] |= NET_6LO_IPHC_SAM_11;
		return inline_ptr;
	}

	/* Following 64 bits are 0000:00ff:fe00:XXXX */
	if (net_6lo_addr_16_bit_compressible(&ipv6->src)) {
		NET_DBG("SAM_10 src addr 16 bit compressible");
		iphc[1] |= NET_6LO_IPHC_SAM_10;

		inline_ptr -= sizeof(u16_t);
		memmove(inline_ptr, &ipv6->src.s6_addr[14], sizeof(u16_t));

		return inline_ptr;
	}

	NET_DBG("SAM_01 src 64 bits are inlined");
	/* Remaining 64 bits are in-line */
	iphc[1] |= NET_6LO_IPHC_SAM_01;

	inline_ptr -= 8U;
	memmove(inline_ptr, &ipv6->src.s6_addr[8], 8U);

	return inline_ptr;
}

static u8_t *set_sa_inline(struct net_ipv6_hdr *ipv6, u8_t *inline_ptr,
			   u8_t *iphc)
{
	iphc[1] |= NET_6LO_IPHC_SAM_00;
	inline_ptr -= 16U;
	memmove(inline_ptr, &ipv6->src.s6_addr[0], 16U);
	return inline_ptr;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
static u8_t *compress_sa_ctx(struct net_ipv6_hdr *ipv6, u8_t *inline_ptr,
			     struct net_pkt *pkt, u8_t *iphc,
			     struct net_6lo_context *src)
{
	NET_ASSERT(net_pkt_lladdr_src(pkt)->addr);

	NET_DBG("SAC_1 src address context based");
	iphc[1] |= NET_6LO_IPHC_SAC_1;

	if (net_ipv6_addr_based_on_ll(&ipv6->src, net_pkt_lladdr_src(pkt))) {
		NET_DBG("SAM_11 src address is fully elided");

		/* Address is fully elided */
		iphc[1] |= NET_6LO_IPHC_SAM_11;
		return inline_ptr;
	}

	/* Following 64 bits are 0000:00ff:fe00:XXXX */
	if (net_6lo_addr_16_bit_compressible(&ipv6->src)) {
		NET_DBG("SAM_10 src addr 16 bit compressible");

		iphc[1] |= NET_6LO_IPHC_SAM_10;

		inline_ptr -= sizeof(u16_t);
		memmove(inline_ptr, &ipv6->src.s6_addr[14], sizeof(u16_t));
		return inline_ptr;
	}

	NET_DBG("SAM_01 src remaining 64 bits are inlined");

	/* Remaining 64 bits are in-line */
	iphc[1] |= NET_6LO_IPHC_SAM_01;

	inline_ptr -= 8U;
	memmove(inline_ptr, &ipv6->src.s6_addr[8], 8U);

	return inline_ptr;
}
#endif

/* Helpers to compress Destination Address */
static u8_t *compress_da_mcast(struct net_ipv6_hdr *ipv6, u8_t *inline_ptr,
			       u8_t *iphc)
{
	iphc[1] |= NET_6LO_IPHC_M_1;

	NET_DBG("M_1 dst is mcast");

	if (net_6lo_maddr_8_bit_compressible(&ipv6->dst)) {
		NET_DBG("DAM_11 dst maddr 8 bit compressible");

		/* last byte */
		iphc[1] |= NET_6LO_IPHC_DAM_11;

		inline_ptr -= sizeof(u8_t);
		memmove(inline_ptr, &ipv6->dst.s6_addr[15], sizeof(u8_t));

		return inline_ptr;
	}

	if (net_6lo_maddr_32_bit_compressible(&ipv6->dst)) {
		NET_DBG("DAM_10 4 bytes: 2nd byte + last three bytes");

		/* 4 bytes: 2nd byte + last three bytes */
		iphc[1] |= NET_6LO_IPHC_DAM_10;

		inline_ptr -= 3U;
		memmove(inline_ptr, &ipv6->dst.s6_addr[13], 3U);

		inline_ptr -= sizeof(u8_t);
		memmove(inline_ptr, &ipv6->dst.s6_addr[1], sizeof(u8_t));

		return inline_ptr;
	}

	if (net_6lo_maddr_48_bit_compressible(&ipv6->dst)) {
		NET_DBG("DAM_01 6 bytes: 2nd byte + last five bytes");

		/* 6 bytes: 2nd byte + last five bytes */
		iphc[1] |= NET_6LO_IPHC_DAM_01;

		inline_ptr -= 5U;
		memmove(inline_ptr, &ipv6->dst.s6_addr[11], 5U);

		inline_ptr -= sizeof(u8_t);
		memmove(inline_ptr, &ipv6->dst.s6_addr[1], sizeof(u8_t));

		return inline_ptr;
	}

	NET_DBG("DAM_00 dst complete addr inlined");

	/* complete address NET_6LO_IPHC_DAM_00 */
	inline_ptr -= 16U;
	memmove(inline_ptr, &ipv6->dst.s6_addr[0], 16U);

	return inline_ptr;
}

static u8_t *compress_da(struct net_ipv6_hdr *ipv6, struct net_pkt *pkt,
			 u8_t *inline_ptr, u8_t *iphc)
{
	NET_ASSERT(net_pkt_lladdr_dst(pkt)->addr);
	/* Address is fully elided */
	if (net_ipv6_addr_based_on_ll(&ipv6->dst, net_pkt_lladdr_dst(pkt))) {
		NET_DBG("DAM_11 dst addr fully elided");

		iphc[1] |= NET_6LO_IPHC_DAM_11;
		return inline_ptr;
	}

	/* Following 64 bits are 0000:00ff:fe00:XXXX */
	if (net_6lo_addr_16_bit_compressible(&ipv6->dst)) {
		NET_DBG("DAM_10 dst addr 16 bit compressible");

		iphc[1] |= NET_6LO_IPHC_DAM_10;

		inline_ptr -= sizeof(u16_t);
		memmove(inline_ptr, &ipv6->dst.s6_addr[14], sizeof(u16_t));
		return inline_ptr;
	}

	NET_DBG("DAM_01 remaining 64 bits are inlined");

	/* Remaining 64 bits are in-line */
	iphc[1] |= NET_6LO_IPHC_DAM_01;

	inline_ptr -= 8U;
	memmove(inline_ptr, &ipv6->dst.s6_addr[8], 8U);

	return inline_ptr;
}

static u8_t *set_da_inline(struct net_ipv6_hdr *ipv6, u8_t *inline_ptr,
			   u8_t *iphc)
{
	iphc[1] |= NET_6LO_IPHC_DAM_00;
	inline_ptr -= 16U;
	memmove(inline_ptr, &ipv6->dst.s6_addr[0], 16U);
	return inline_ptr;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
static u8_t *compress_da_ctx(struct net_ipv6_hdr *ipv6, u8_t *inline_ptr,
			     struct net_pkt *pkt, u8_t *iphc,
			     struct net_6lo_context *dst)
{
	iphc[1] |= NET_6LO_IPHC_DAC_1;

	if (net_ipv6_addr_based_on_ll(&ipv6->dst, net_pkt_lladdr_dst(pkt))) {
		NET_DBG("DAM_11 dst addr fully elided");

		iphc[1] |= NET_6LO_IPHC_DAM_11;
		return inline_ptr;
	}

	/* Following 64 bits are 0000:00ff:fe00:XXXX */
	if (net_6lo_addr_16_bit_compressible(&ipv6->dst)) {
		NET_DBG("DAM_10 dst addr 16 bit compressible");

		iphc[1] |= NET_6LO_IPHC_DAM_10;
		inline_ptr -= sizeof(u16_t);
		memmove(inline_ptr, &ipv6->dst.s6_addr[14], sizeof(u16_t));
		return inline_ptr;
	}

	NET_DBG("DAM_01 remaining 64 bits are inlined");

	/* Remaining 64 bits are in-line */
	iphc[1] |= NET_6LO_IPHC_DAM_01;

	inline_ptr -= 8U;
	memmove(inline_ptr, &ipv6->dst.s6_addr[8], 8U);

	return inline_ptr;
}
#endif

/* Helper to compress Next header UDP */
static inline u8_t *compress_nh_udp(struct net_udp_hdr *udp, u8_t *inline_ptr,
				    bool compress_checksum)
{
	u8_t nhc = NET_6LO_NHC_UDP_BARE;
	u8_t *inline_ptr_udp = inline_ptr;
	u8_t tmp;

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

		tmp = (u8_t)(htons(udp->src_port));
		tmp = tmp << 4;

		tmp |= (((u8_t)(htons(udp->dst_port))) & 0x0F);
		inline_ptr_udp -= sizeof(tmp);
		*inline_ptr_udp = tmp;

	} else if (((htons(udp->dst_port) >> 8) & 0xFF) ==
		   NET_6LO_NHC_UDP_8_BIT_PORT) {

		NET_DBG("UDP ports src full, dst 8 bits inlined");
		/* dst: first 8 bits elided, next 8 bits inlined
		 * src: fully carried inline
		 */
		nhc |= NET_6LO_NHC_UDP_PORT_01;

		inline_ptr_udp -= sizeof(u8_t);
		*inline_ptr_udp = (u8_t)(htons(udp->dst_port));

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

		inline_ptr_udp -= sizeof(u8_t);
		*inline_ptr_udp = (u8_t)(htons(udp->src_port));

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

	src = get_6lo_context_by_addr(net_pkt_iface(pkt), &ipv6->src);
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

	dst = get_6lo_context_by_addr(net_pkt_iface(pkt), &ipv6->dst);
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
	u8_t compressed = 0;
	u8_t iphc[2] = {NET_6LO_DISPATCH_IPHC, 0};
	struct net_ipv6_hdr *ipv6 = NET_IPV6_HDR(pkt);
	struct net_udp_hdr *udp;
	u8_t *inline_pos;

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

	if (net_6lo_ll_prefix_padded_with_zeros(&ipv6->dst)) {
		inline_pos = compress_da(ipv6, pkt, inline_pos, iphc);
		goto da_end;
	}

	if (net_ipv6_is_addr_mcast(&ipv6->dst)) {
		inline_pos = compress_da_mcast(ipv6, inline_pos, iphc);
		goto da_end;
	}

#if defined(CONFIG_NET_6LO_CONTEXT)
	dst_ctx = get_dst_addr_ctx(pkt, ipv6);
	if (dst_ctx) {
		iphc[1] |= NET_6LO_IPHC_CID_1;
		inline_pos = compress_da_ctx(ipv6, inline_pos, pkt, iphc,
					     dst_ctx);
		goto da_end;
	}
#endif
	inline_pos = set_da_inline(ipv6, inline_pos, iphc);
da_end:

	if (net_6lo_ll_prefix_padded_with_zeros(&ipv6->src)) {
		inline_pos = compress_sa(ipv6, pkt, inline_pos, iphc);
		goto sa_end;
	}

	if (net_ipv6_is_addr_unspecified(&ipv6->src)) {
		NET_DBG("SAM_00, SAC_1 unspecified src address");

		/* Unspecified IPv6 src address */
		iphc[1] |= NET_6LO_IPHC_SAC_1;
		iphc[1] |= NET_6LO_IPHC_SAM_00;
		goto sa_end;
	}

#if defined(CONFIG_NET_6LO_CONTEXT)
	src_ctx = get_src_addr_ctx(pkt, ipv6);
	if (src_ctx) {
		inline_pos = compress_sa_ctx(ipv6, inline_pos, pkt, iphc,
					     src_ctx);
		iphc[1] |= NET_6LO_IPHC_CID_1;
		goto sa_end;
	}
#endif
	inline_pos = set_sa_inline(ipv6, inline_pos, iphc);
sa_end:

	inline_pos = compress_hoplimit(ipv6, inline_pos, iphc);
	inline_pos = compress_nh(ipv6, inline_pos, iphc);
	inline_pos = compress_tfl(ipv6, inline_pos, iphc);

#if defined(CONFIG_NET_6LO_CONTEXT)
	if (iphc[1] & NET_6LO_IPHC_CID_1) {
		inline_pos -= sizeof(u8_t);
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
	memmove(inline_pos, iphc, sizeof(iphc));

	compressed = inline_pos - pkt->buffer->data;

	net_buf_pull(pkt->buffer, compressed);

	return compressed;
}

/* Helper to uncompress Traffic class and Flow label */
static inline u8_t uncompress_tfl(struct net_pkt *pkt,
				  struct net_ipv6_hdr *ipv6,
				  u8_t offset, bool dry_run)
{
	u8_t tcl;

	/* Uncompress tcl and flow label */
	switch (CIPHC[0] & NET_6LO_IPHC_TF_11) {
	case NET_6LO_IPHC_TF_00:
		NET_DBG("ECN + DSCP + 4-bit Pad + Flow Label");

		tcl = CIPHC[offset];
		tcl = (tcl >> 6) | (tcl << 2);

		if (!dry_run) {
			ipv6->vtc |= ((tcl & 0xF0) >> 4);
			ipv6->tcflow = ((tcl & 0x0F) << 4) |
				(CIPHC[offset + 1] & 0x0F);

			memcpy(&ipv6->flow, &CIPHC[offset + 2U], 2);
		}

		offset += 4U;
		break;
	case NET_6LO_IPHC_TF_01:
		NET_DBG("ECN + 2-bit Pad + Flow Label, DSCP is elided");

		if (!dry_run) {
			tcl = ((CIPHC[offset] & 0xF0) >> 6);
			ipv6->tcflow = ((tcl & 0x0F) << 4) |
				(CIPHC[offset] & 0x0F);

			memcpy(&ipv6->flow, &CIPHC[offset + 1], 2);
		}

		offset += 3U;
		break;
	case NET_6LO_IPHC_TF_10:
		NET_DBG("Flow label elided");

		if (!dry_run) {
			tcl = CIPHC[offset];
			tcl = (tcl >> 6) | (tcl << 2);

			ipv6->vtc |= ((tcl & 0xF0) >> 4);
			ipv6->tcflow = (tcl & 0x0F) << 4;
			ipv6->flow = 0U;
		}

		offset++;
		break;
	case NET_6LO_IPHC_TF_11:
		NET_DBG("Tcl and Flow label elided");

		if (!dry_run) {
			ipv6->tcflow = 0U;
			ipv6->flow = 0U;
		}

		break;
	}

	return offset;
}

/* Helper to uncompress Hoplimit */
static inline u8_t uncompress_hoplimit(struct net_pkt *pkt,
				       struct net_ipv6_hdr *ipv6,
				       u8_t offset, bool dry_run)
{
	switch (CIPHC[0] & NET_6LO_IPHC_HLIM255) {
	case NET_6LO_IPHC_HLIM:
		if (!dry_run) {
			ipv6->hop_limit = CIPHC[offset];
		}

		offset++;
		break;
	case NET_6LO_IPHC_HLIM1:
		if (!dry_run) {
			ipv6->hop_limit = 1U;
		}

		break;
	case NET_6LO_IPHC_HLIM64:
		if (!dry_run) {
			ipv6->hop_limit = 64U;
		}

		break;
	case NET_6LO_IPHC_HLIM255:
		if (!dry_run) {
			ipv6->hop_limit = 255U;
		}

		break;
	}

	return offset;
}

/* Helper to uncompress Source Address */
static inline u8_t uncompress_sa(struct net_pkt *pkt,
				 struct net_ipv6_hdr *ipv6,
				 u8_t offset, bool dry_run)
{

	NET_DBG("SAC_0");

	switch (CIPHC[1] & NET_6LO_IPHC_SAM_11) {
	case NET_6LO_IPHC_SAM_00:
		NET_DBG("SAM_00 full src addr inlined");

		if (!dry_run) {
			memcpy(ipv6->src.s6_addr, &CIPHC[offset], 16);
		}

		offset += 16U;
		break;
	case NET_6LO_IPHC_SAM_01:
		NET_DBG("SAM_01 last 64 bits are inlined");

		if (!dry_run) {
			ipv6->src.s6_addr[0] = 0xFE;
			ipv6->src.s6_addr[1] = 0x80;

			memcpy(&ipv6->src.s6_addr[8], &CIPHC[offset], 8);
		}

		offset += 8U;
		break;
	case NET_6LO_IPHC_SAM_10:
		NET_DBG("SAM_10 src addr 16 bit compressed");

		if (!dry_run) {
			ipv6->src.s6_addr[0] = 0xFE;
			ipv6->src.s6_addr[1] = 0x80;
			ipv6->src.s6_addr[11] = 0xFF;
			ipv6->src.s6_addr[12] = 0xFE;

			memcpy(&ipv6->src.s6_addr[14], &CIPHC[offset], 2);
		}

		offset += 2U;
		break;
	case NET_6LO_IPHC_SAM_11:
		NET_DBG("SAM_11 generate src addr from ll");

		if (!dry_run) {
			net_ipv6_addr_create_iid(&ipv6->src,
						 net_pkt_lladdr_src(pkt));
		}

		break;
	}

	return offset;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
static inline u8_t uncompress_sa_ctx(struct net_pkt *pkt,
				     struct net_ipv6_hdr *ipv6,
				     u8_t offset,
				     struct net_6lo_context *ctx,
				     bool dry_run)
{
	switch (CIPHC[1] & NET_6LO_IPHC_SAM_11) {
	case NET_6LO_IPHC_SAM_01:
		NET_DBG("SAM_01 last 64 bits are inlined");

		if (!dry_run) {
			/* First 8 bytes are from context */
			memcpy(&ipv6->src.s6_addr[0],
			       &ctx->prefix.s6_addr[0], 8);

			/* And the rest are carried in-line*/
			memcpy(&ipv6->src.s6_addr[8], &CIPHC[offset], 8);
		}

		offset += 8U;

		break;
	case NET_6LO_IPHC_SAM_10:
		NET_DBG("SAM_10 src addr 16 bit compressed");

		if (!dry_run) {
			/* First 8 bytes are from context */
			memcpy(&ipv6->src.s6_addr[0],
			       &ctx->prefix.s6_addr[0], 8);

			ipv6->src.s6_addr[11] = 0xFF;
			ipv6->src.s6_addr[12] = 0xFE;

			/* And the rest are carried in-line */
			memcpy(&ipv6->src.s6_addr[14], &CIPHC[offset], 2);
		}

		offset += 2U;

		break;
	case NET_6LO_IPHC_SAM_11:
		NET_DBG("SAM_11 generate src addr from ll");

		if (dry_run) {
			break;
		}

		/* RFC 6282, 3.1.1. If SAC = 1 and SAM = 11
		 * Derive addr using context information and
		 * the encapsulating header.
		 * (e.g., 802.15.4 or IPv6 source address).
		 */
		net_ipv6_addr_create_iid(&ipv6->src, net_pkt_lladdr_src(pkt));

		/* net_ipv6_addr_create_iid will copy first 8 bytes
		 * as link local prefix.
		 * Overwrite first 8 bytes from context prefix here.
		 */
		memcpy(&ipv6->src.s6_addr[0], &ctx->prefix.s6_addr[0], 8);
		break;
	}

	return offset;
}
#endif

/* Helpers to uncompress Destination Address */
static inline u8_t uncompress_da_mcast(struct net_pkt *pkt,
				       struct net_ipv6_hdr *ipv6,
				       u8_t offset, bool dry_run)
{
	NET_DBG("Dst is multicast");

	if (CIPHC[1] & NET_6LO_IPHC_DAC_1) {
		NET_WARN("Unsupported DAM options");
		return 0;
	}

	/* If M=1 and DAC=0:
	 * 00: 128 bits, The full address is carried in-line.
	 * 01:  48 bits, The address takes the form ffXX::00XX:XXXX:XXXX.
	 * 10:  32 bits, The address takes the form ffXX::00XX:XXXX.
	 * 11:   8 bits, The address takes the form ff02::00XX.
	 */

	switch (CIPHC[1] & NET_6LO_IPHC_DAM_11) {
	case NET_6LO_IPHC_DAM_00:
		NET_DBG("DAM_00 full dst addr inlined");

		if (!dry_run) {
			memcpy(&ipv6->dst.s6_addr[0], &CIPHC[offset], 16);
		}

		offset += 16U;
		break;
	case NET_6LO_IPHC_DAM_01:
		NET_DBG("DAM_01 2nd byte and last five bytes");

		if (!dry_run) {
			ipv6->dst.s6_addr[0] = 0xFF;
			ipv6->dst.s6_addr[1] = CIPHC[offset];

			memcpy(&ipv6->dst.s6_addr[11], &CIPHC[offset + 1], 5);
		}

		offset += 6U;
		break;
	case NET_6LO_IPHC_DAM_10:
		NET_DBG("DAM_10 2nd byte and last three bytes");

		if (!dry_run) {
			ipv6->dst.s6_addr[0] = 0xFF;
			ipv6->dst.s6_addr[1] = CIPHC[offset];

			memcpy(&ipv6->dst.s6_addr[13], &CIPHC[offset + 1], 3);
		}

		offset += 4U;
		break;
	case NET_6LO_IPHC_DAM_11:
		NET_DBG("DAM_11 8 bit compressed");

		if (!dry_run) {
			ipv6->dst.s6_addr[0] = 0xFF;
			ipv6->dst.s6_addr[1] = 0x02;
			ipv6->dst.s6_addr[15] = CIPHC[offset];
		}

		offset++;

		break;
	}

	return offset;
}

/* Helper to uncompress Destination Address */
static inline u8_t uncompress_da(struct net_pkt *pkt,
				 struct net_ipv6_hdr *ipv6,
				 u8_t offset, bool dry_run)
{
	NET_DBG("DAC_0");

	if (CIPHC[1] & NET_6LO_IPHC_M_1) {
		return uncompress_da_mcast(pkt, ipv6, offset, dry_run);
	}

	switch (CIPHC[1] & NET_6LO_IPHC_DAM_11) {
	case NET_6LO_IPHC_DAM_00:
		NET_DBG("DAM_00 full dst addr inlined");

		if (!dry_run) {
			memcpy(&ipv6->dst.s6_addr[0], &CIPHC[offset], 16);
		}

		offset += 16U;
		break;
	case NET_6LO_IPHC_DAM_01:
		NET_DBG("DAM_01 last 64 bits are inlined");

		if (!dry_run) {
			ipv6->dst.s6_addr[0] = 0xFE;
			ipv6->dst.s6_addr[1] = 0x80;

			memcpy(&ipv6->dst.s6_addr[8], &CIPHC[offset], 8);
		}

		offset += 8U;
		break;
	case NET_6LO_IPHC_DAM_10:
		NET_DBG("DAM_10 dst addr 16 bit compressed");

		if (!dry_run) {
			ipv6->dst.s6_addr[0] = 0xFE;
			ipv6->dst.s6_addr[1] = 0x80;
			ipv6->dst.s6_addr[11] = 0xFF;
			ipv6->dst.s6_addr[12] = 0xFE;

			memcpy(&ipv6->dst.s6_addr[14], &CIPHC[offset], 2);
		}

		offset += 2U;
		break;
	case NET_6LO_IPHC_DAM_11:
		NET_DBG("DAM_11 generate dst addr from ll");

		if (!dry_run) {
			net_ipv6_addr_create_iid(&ipv6->dst,
						 net_pkt_lladdr_dst(pkt));
		}

		break;
	}

	return offset;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
static inline u8_t uncompress_da_ctx(struct net_pkt *pkt,
				     struct net_ipv6_hdr *ipv6,
				     u8_t offset,
				     struct net_6lo_context *ctx,
				     bool dry_run)
{
	NET_DBG("DAC_1");

	if (CIPHC[1] & NET_6LO_IPHC_M_1) {
		return uncompress_da_mcast(pkt, ipv6, offset, dry_run);
	}

	switch (CIPHC[1] & NET_6LO_IPHC_DAM_11) {
	case NET_6LO_IPHC_DAM_01:
		NET_DBG("DAM_01 last 64 bits are inlined");

		if (!dry_run) {
			/* First 8 bytes are from context */
			memcpy(&ipv6->dst.s6_addr[0],
			       &ctx->prefix.s6_addr[0], 8);

			/* And the rest are carried in-line */
			memcpy(&ipv6->dst.s6_addr[8], &CIPHC[offset], 8);
		}

		offset += 8U;

		break;
	case NET_6LO_IPHC_DAM_10:
		NET_DBG("DAM_10 src addr 16 bit compressed");

		if (!dry_run) {
			/* First 8 bytes are from context */
			memcpy(&ipv6->dst.s6_addr[0],
			       &ctx->prefix.s6_addr[0], 8);

			ipv6->dst.s6_addr[11] = 0xFF;
			ipv6->dst.s6_addr[12] = 0xFE;

			/* And the restare carried in-line */
			memcpy(&ipv6->dst.s6_addr[14], &CIPHC[offset], 2);
		}

		offset += 2U;

		break;
	case NET_6LO_IPHC_DAM_11:
		NET_DBG("DAM_11 generate src addr from ll");

		if (dry_run) {
			break;
		}

		/* RFC 6282, 3.1.1. If SAC = 1 and SAM = 11
		 * Derive addr using context information and
		 * the encapsulating header.
		 * (e.g., 802.15.4 or IPv6 source address).
		 */
		net_ipv6_addr_create_iid(&ipv6->dst, net_pkt_lladdr_dst(pkt));

		/* net_ipv6_addr_create_iid will copy first 8 bytes
		 * as link local prefix.
		 * Overwrite first 8 bytes from context prefix here.
		 */
		memcpy(&ipv6->dst.s6_addr[0], &ctx->prefix.s6_addr[0], 8);

		break;
	}

	return offset;
}
#endif

/* Helper to uncompress NH UDP */
static inline u8_t uncompress_nh_udp(struct net_pkt *pkt,
				     struct net_udp_hdr *udp,
				     u8_t offset, bool dry_run)
{
	/* Port uncompression
	 * 00:  All 16 bits for src and dst are inlined
	 * 01: src, 16 bits are lined, dst(0xf0) 8 bits are inlined
	 * 10: dst, 16 bits are lined, src(0xf0) 8 bits are inlined
	 * 11: src, dst (0xf0b) 4 bits are inlined
	 */

	/* UDP header uncompression */
	switch (CIPHC[offset++] & NET_6LO_NHC_UDP_PORT_11) {
	case NET_6LO_NHC_UDP_PORT_00:
		NET_DBG("src and dst ports are inlined");

		if (!dry_run) {
			memcpy(&udp->src_port, &CIPHC[offset], 2);
			memcpy(&udp->dst_port, &CIPHC[offset + 2U], 2);
		}

		offset += 4U;
		break;
	case NET_6LO_NHC_UDP_PORT_01:
		NET_DBG("src full, dst 8 bits inlined");

		if (!dry_run) {
			memcpy(&udp->src_port, &CIPHC[offset], 2);
			udp->dst_port = htons(((u16_t)NET_6LO_NHC_UDP_8_BIT_PORT
					       << 8) | CIPHC[offset + 2U]);
		}

		offset += 3U;
		break;
	case NET_6LO_NHC_UDP_PORT_10:
		NET_DBG("src 8 bits, dst full inlined");

		if (!dry_run) {
			udp->src_port = htons(((u16_t)NET_6LO_NHC_UDP_8_BIT_PORT
					       << 8) | CIPHC[offset]);
			memcpy(&udp->dst_port, &CIPHC[offset + 1], 2);
		}

		offset += 3U;
		break;
	case NET_6LO_NHC_UDP_PORT_11:
		NET_DBG("src and dst 4 bits inlined");

		if (!dry_run) {
			udp->src_port = htons(
				(NET_6LO_NHC_UDP_4_BIT_PORT << 4) |
				(CIPHC[offset] >> 4));

			udp->dst_port = htons(
				(NET_6LO_NHC_UDP_4_BIT_PORT << 4) |
				(CIPHC[offset] & 0x0F));
		}

		offset++;
		break;
	}

	return offset;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
/* Helper function to uncompress src and dst contexts */
static inline void uncompress_cid(struct net_pkt *pkt,
				  struct net_6lo_context **src,
				  struct net_6lo_context **dst)
{
	u8_t cid;

	/* Extract source and destination Context Index,
	 * Either src or dest address is context based or both.
	 */
	cid = (CIPHC[2] >> 4) & 0x0F;
	*src = get_6lo_context_by_cid(net_pkt_iface(pkt), cid);
	if (!(*src)) {
		NET_DBG("Unknown src cid %d", cid);
	}

	cid = CIPHC[2] & 0x0F;
	*dst = get_6lo_context_by_cid(net_pkt_iface(pkt), cid);
	if (!(*dst)) {
		NET_DBG("Unknown dst cid %d", cid);
	}
}
#endif

static bool uncompress_IPHC_header(struct net_pkt *pkt,
				   bool dry_run, int *diff)
{
	struct net_udp_hdr *udp = NULL;
	u8_t offset = 2U;
	u8_t chksum = 0U;
	struct net_buf *frag = NULL;
	struct net_ipv6_hdr *ipv6;
	u16_t len;
#if defined(CONFIG_NET_6LO_CONTEXT)
	struct net_6lo_context *src = NULL;
	struct net_6lo_context *dst = NULL;
#endif

	if (dry_run && !diff) {
		return false;
	}

	if (CIPHC[1] & NET_6LO_IPHC_CID_1) {
#if defined(CONFIG_NET_6LO_CONTEXT)
		uncompress_cid(pkt, &src, &dst);
		offset++;
#else
		NET_WARN("Context based uncompression not enabled");
		return false;
#endif
	}

	if (!dry_run) {
		frag = net_pkt_get_frag(pkt, NET_6LO_RX_PKT_TIMEOUT);
		if (!frag) {
			return false;
		}

		ipv6 = (struct net_ipv6_hdr *)(frag->data);
	} else {
		/* This is meant to avoid compiler warnings: that area
		 * will not be modified.
		 */
		ipv6 = (struct net_ipv6_hdr *)(pkt->buffer->data);
	}

	/* Version is always 6 */
	if (!dry_run) {
		ipv6->vtc = 0x60;
		net_pkt_set_ip_hdr_len(pkt, NET_IPV6H_LEN);
	}

	/* Uncompress Traffic class and Flow label */
	offset = uncompress_tfl(pkt, ipv6, offset, dry_run);

	if (!(CIPHC[0] & NET_6LO_IPHC_NH_1)) {
		if (!dry_run) {
			ipv6->nexthdr = CIPHC[offset];
		}

		offset++;
	}

	/* Uncompress Hoplimit */
	offset = uncompress_hoplimit(pkt, ipv6, offset, dry_run);

	/* First set to zero and copy relevant bits */
	if (!dry_run) {
		(void)memset(&ipv6->src.s6_addr[0], 0, 16);
		(void)memset(&ipv6->dst.s6_addr[0], 0, 16);
	}

	/* Uncompress Source Address */
	if (CIPHC[1] & NET_6LO_IPHC_SAC_1) {
		NET_DBG("SAC_1");

		if ((CIPHC[1] & NET_6LO_IPHC_SAM_11) == NET_6LO_IPHC_SAM_00) {
			NET_DBG("SAM_00 unspecified address");
		} else {
#if defined(CONFIG_NET_6LO_CONTEXT)
			if (!src) {
				NET_ERR("Src context doesn't exists");
				goto fail;
			}

			offset = uncompress_sa_ctx(pkt, ipv6,
						   offset, src, dry_run);
#else
			NET_WARN("Context based uncompression not enabled");
			goto fail;
#endif
		}
	} else {
		offset = uncompress_sa(pkt, ipv6, offset, dry_run);
	}

	/* Uncompress Destination Address */
#if defined(CONFIG_NET_6LO_CONTEXT)
	if (CIPHC[1] & NET_6LO_IPHC_DAC_1) {
		if (CIPHC[1] & NET_6LO_IPHC_M_1) {
			/* TODO: DAM00 Unicast-Prefix-based IPv6 Multicast
			 * Addresses. DAM_01, DAM_10 and DAM_11 are reserved.
			 */
			NET_ERR("DAC_1 and M_1 is not supported");
			goto fail;
		}

		if (!dst) {
			NET_ERR("DAC is set but dst context doesn't exists");
			goto fail;
		}

		offset = uncompress_da_ctx(pkt, ipv6, offset, dst, dry_run);
	} else {
		offset = uncompress_da(pkt, ipv6, offset, dry_run);
	}
#else
	offset = uncompress_da(pkt, ipv6, offset, dry_run);
#endif

	if (!dry_run) {
		net_buf_add(frag, NET_IPV6H_LEN);
	} else {
		*diff = NET_IPV6H_LEN;
	}

	if (!(CIPHC[0] & NET_6LO_IPHC_NH_1)) {
		NET_DBG("No following compressed header");
		goto end;
	}

	if ((CIPHC[offset] & 0xF0) != NET_6LO_NHC_UDP_BARE) {
		/* Unsupported NH,
		 * Supports only UDP header (next header) compression.
		 */
		NET_ERR("Unsupported next header");
		goto fail;
	}

	/* Uncompress UDP header */
	if (!dry_run) {
		ipv6->nexthdr = IPPROTO_UDP;

		udp = (struct net_udp_hdr *)(frag->data + NET_IPV6H_LEN);
	}

	chksum = CIPHC[offset] & NET_6LO_NHC_UDP_CHKSUM_1;
	offset = uncompress_nh_udp(pkt, udp, offset, dry_run);

	if (!chksum) {
		if (!dry_run) {
			memcpy(&udp->chksum, &CIPHC[offset], 2);
		}

		offset += 2U;
	}

	if (!dry_run) {
		net_buf_add(frag, NET_UDPH_LEN);
	} else {
		*diff += NET_UDPH_LEN;
	}

end:
	if (pkt->frags->len < offset) {
		NET_ERR("pkt %p too short len %d vs %d", pkt,
			pkt->frags->len, offset);
		goto fail;
	}

	if (dry_run) {
		/* We set the difference of header sizes */
		*diff -= offset;
		return true;
	}

	/* Move the data to beginning, no need for headers now */
	NET_DBG("Removing %u bytes of compressed hdr", offset);
	memmove(pkt->frags->data, pkt->frags->data + offset,
		pkt->frags->len - offset);
	pkt->frags->len -= offset;

	/* Insert the fragment (this one holds uncompressed headers) */
	net_pkt_frag_insert(pkt, frag);
	net_pkt_compact(pkt);

	/* Set IPv6 header and UDP (if next header is) length */
	len = net_pkt_get_len(pkt) - NET_IPV6H_LEN;
	ipv6->len = htons(len);

	if (ipv6->nexthdr == IPPROTO_UDP && udp) {
		udp->len = htons(len);

		if (chksum) {
			udp->chksum = net_calc_chksum_udp(pkt);
		}
	}

	return true;

fail:
	if (frag) {
		net_pkt_frag_unref(frag);
	}

	return false;
}

/* Adds IPv6 dispatch as first byte and adjust fragments  */
static inline int compress_ipv6_header(struct net_pkt *pkt)
{
	struct net_buf *frag;

	frag = net_pkt_get_frag(pkt, K_FOREVER);
	if (!frag) {
		return -ENOBUFS;
	}

	frag->data[0] = NET_6LO_DISPATCH_IPV6;
	net_buf_add(frag, 1);

	net_pkt_frag_insert(pkt, frag);

	/* Compact the fragments, so that gaps will be filled */
	net_pkt_compact(pkt);

	return 0;
}

static inline bool uncompress_ipv6_header(struct net_pkt *pkt)
{
	struct net_buf *frag = pkt->frags;

	/* Pull off IPv6 dispatch header and adjust data and length */
	memmove(frag->data, frag->data + 1, frag->len - 1U);
	frag->len -= 1U;

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

	if ((pkt->frags->data[0] & NET_6LO_DISPATCH_IPHC) ==
	    NET_6LO_DISPATCH_IPHC) {
		/* Uncompress IPHC header */
		return uncompress_IPHC_header(pkt, false, NULL);

	} else if ((pkt->frags->data[0] & NET_6LO_DISPATCH_IPV6) ==
		   NET_6LO_DISPATCH_IPV6) {
		/* Uncompress IPv6 header, it has only IPv6 dispatch in the
		 * beginning */
		return uncompress_ipv6_header(pkt);
	}

	NET_DBG("pkt %p is not compressed", pkt);

	return true;
}

int net_6lo_uncompress_hdr_diff(struct net_pkt *pkt)
{
	if ((pkt->frags->data[0] & NET_6LO_DISPATCH_IPHC) ==
	    NET_6LO_DISPATCH_IPHC) {
		int len;

		if (!uncompress_IPHC_header(pkt, true, &len)) {
			return INT_MAX;
		}

		return len;
	} else if ((pkt->frags->data[0] & NET_6LO_DISPATCH_IPV6) ==
		   NET_6LO_DISPATCH_IPV6) {
		return -1;
	}

	return 0;
}
