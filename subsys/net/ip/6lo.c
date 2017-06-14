/** @file
 * @brief 6lopan related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_6LO)
#define SYS_LOG_DOMAIN "net/6lo"
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>
#include <net/net_core.h>
#include <net/net_if.h>
#include <net/net_stats.h>

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
	return ((UNALIGNED_GET(&addr->s6_addr16[1]) == 0x00) &&
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
	for (i = 0; i < CONFIG_NET_MAX_6LO_CONTEXTS; i++) {
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

	for (i = 0; i < CONFIG_NET_MAX_6LO_CONTEXTS; i++) {
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

	for (i = 0; i < CONFIG_NET_MAX_6LO_CONTEXTS; i++) {
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
static inline u8_t compress_tfl(struct net_ipv6_hdr *ipv6,
				   struct net_buf *frag,
				   u8_t offset)
{
	u8_t tcl;

	tcl = ((ipv6->vtc & 0x0F) << 4) | ((ipv6->tcflow & 0xF0) >> 4);
	tcl = (tcl << 6) | (tcl >> 2);   /* ECN(2), DSCP(6) */

	if (((ipv6->tcflow & 0x0F) == 0) && (ipv6->flow == 0)) {
		if (((ipv6->vtc & 0x0F) == 0) && ((ipv6->tcflow & 0xF0) == 0)) {
			NET_DBG("Trafic class and Flow label elided");

			/* Trafic class and Flow label elided */
			IPHC[0] |= NET_6LO_IPHC_TF_11;
		} else {
			NET_DBG("Flow label elided");

			/* Flow label elided */
			IPHC[0] |= NET_6LO_IPHC_TF_10;
			IPHC[offset++] = tcl;
		}
	} else {
		if (((ipv6->vtc & 0x0F) == 0) && (ipv6->tcflow & 0x30)) {
			NET_DBG("ECN + 2-bit Pad + Flow Label, DSCP is elided");

			/* ECN + 2-bit Pad + Flow Label, DSCP is elided.*/
			IPHC[0] |= NET_6LO_IPHC_TF_01;
			IPHC[offset++] = (tcl & 0xC0) | (ipv6->tcflow & 0x0F);

			memcpy(&IPHC[offset], &ipv6->flow, 2);
			offset += 2;
		} else {
			NET_DBG("ECN + DSCP + 4-bit Pad + Flow Label");

			/* ECN + DSCP + 4-bit Pad + Flow Label */
			IPHC[0] |= NET_6LO_IPHC_TF_00;

			/* Elide the version field */
			IPHC[offset++] = tcl;
			IPHC[offset++] = ipv6->tcflow & 0x0F;

			memcpy(&IPHC[offset], &ipv6->flow, 2);
			offset += 2;
		}
	}

	return offset;
}

/* Helper to compress Hop limit */
static inline u8_t compress_hoplimit(struct net_ipv6_hdr *ipv6,
					struct net_buf *frag,
					u8_t offset)
{
	/* Hop Limit */
	switch (ipv6->hop_limit) {
	case 1:
		IPHC[0] |= NET_6LO_IPHC_HLIM1;
		break;
	case 64:
		IPHC[0] |= NET_6LO_IPHC_HLIM64;
		break;
	case 255:
		IPHC[0] |= NET_6LO_IPHC_HLIM255;
		break;
	default:
		IPHC[offset++] = ipv6->hop_limit;
		break;
	}

	return offset;
}

/* Helper to compress Next header */
static inline u8_t compress_nh(struct net_ipv6_hdr *ipv6,
				  struct net_buf *frag, u8_t offset)
{
	/* Next header */
	if (ipv6->nexthdr == IPPROTO_UDP) {
		IPHC[0] |= NET_6LO_IPHC_NH_1;
	} else {
		IPHC[offset++] = ipv6->nexthdr;
	}

	return offset;
}

/* Helpers to compress Source Address */
static inline u8_t compress_sa(struct net_ipv6_hdr *ipv6,
				  struct net_pkt *pkt,
				  struct net_buf *frag,
				  u8_t offset)
{
	if (net_is_ipv6_addr_unspecified(&ipv6->src)) {
		NET_DBG("SAM_00, SAC_1 unspecified src address");

		/* Unspecified IPv6 src address */
		IPHC[1] |= NET_6LO_IPHC_SAC_1;
		IPHC[1] |= NET_6LO_IPHC_SAM_00;

		return offset;
	}

	/* If address is link-local prefix and padded with zeros */
	if (net_is_ipv6_ll_addr(&ipv6->src) &&
	    net_6lo_ll_prefix_padded_with_zeros(&ipv6->src)) {

		NET_DBG("SAC_0 src is ll_addr and padded with zeros");

		/* Following 64 bits are 0000:00ff:fe00:XXXX */
		if (net_6lo_addr_16_bit_compressible(&ipv6->src)) {
			NET_DBG("SAM_10 src addr 16 bit compressible");

			IPHC[1] |= NET_6LO_IPHC_SAM_10;

			memcpy(&IPHC[offset], &ipv6->src.s6_addr[14], 2);
			offset += 2;
		} else {
			if (!net_pkt_ll_src(pkt)) {
				NET_ERR("Invalid src ll address");
				return 0;
			}

			if (net_ipv6_addr_based_on_ll(&ipv6->src,
						      net_pkt_ll_src(pkt))) {
				NET_DBG("SAM_11 src address is fully elided");

				/* Address is fully elided */
				IPHC[1] |= NET_6LO_IPHC_SAM_11;
			} else {
				NET_DBG("SAM_01 src 64 bits are inlined");

				/* Remaining 64 bits are in-line */
				IPHC[1] |= NET_6LO_IPHC_SAM_01;

				memcpy(&IPHC[offset], &ipv6->src.s6_addr[8], 8);
				offset += 8;
			}
		}
	} else {
		NET_DBG("SAM_00 full src address is carried in-line");
		/* full address is carried in-line */
		IPHC[1] |= NET_6LO_IPHC_SAM_00;

		memcpy(&IPHC[offset], ipv6->src.s6_addr,
		       sizeof(struct in6_addr));
		offset += sizeof(struct in6_addr);
	}

	return offset;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
static inline u8_t compress_sa_ctx(struct net_ipv6_hdr *ipv6,
				      struct net_pkt *pkt,
				      struct net_buf *frag,
				      u8_t offset,
				      struct net_6lo_context *src)
{
	if (!src) {
		return compress_sa(ipv6, pkt, frag, offset);
	}

	IPHC[1] |= NET_6LO_IPHC_SAC_1;

	/* Following 64 bits are 0000:00ff:fe00:XXXX */
	if (net_6lo_addr_16_bit_compressible(&ipv6->src)) {
		NET_DBG("SAM_10 src addr 16 bit compressible");

		IPHC[1] |= NET_6LO_IPHC_SAM_10;

		memcpy(&IPHC[offset], &ipv6->src.s6_addr[14], 2);
		offset += 2;
	} else if (net_ipv6_addr_based_on_ll(&ipv6->src,
					     net_pkt_ll_src(pkt))) {
		NET_DBG("SAM_11 src address is fully elided");

		/* Address is fully elided */
		IPHC[1] |= NET_6LO_IPHC_SAM_11;
	} else {
		NET_DBG("SAM_01 src remaining 64 bits are inlined");

		/* Remaining 64 bits are in-line */
		IPHC[1] |= NET_6LO_IPHC_SAM_01;

		memcpy(&IPHC[offset], &ipv6->src.s6_addr[8], 8);
		offset += 8;
	}

	return offset;
}
#endif

/* Helpers to compress Destination Address */
static inline u8_t compress_da_mcast(struct net_ipv6_hdr *ipv6,
					struct net_pkt *pkt,
					struct net_buf *frag,
					u8_t offset)
{
	IPHC[1] |= NET_6LO_IPHC_M_1;

	NET_DBG("M_1 dst is mcast");

	if (net_6lo_maddr_8_bit_compressible(&ipv6->dst)) {
		NET_DBG("DAM_11 dst maddr 8 bit compressible");

		/* last byte */
		IPHC[1] |= NET_6LO_IPHC_DAM_11;

		memcpy(&IPHC[offset], &ipv6->dst.s6_addr[15], 1);
		offset++;
	} else if (net_6lo_maddr_32_bit_compressible(&ipv6->dst)) {
		NET_DBG("DAM_10 4 bytes: 2nd byte + last three bytes");

		/* 4 bytes: 2nd byte + last three bytes */
		IPHC[1] |= NET_6LO_IPHC_DAM_10;

		memcpy(&IPHC[offset], &ipv6->dst.s6_addr[1], 1);
		offset++;

		memcpy(&IPHC[offset], &ipv6->dst.s6_addr[13], 3);
		offset += 3;
	} else if (net_6lo_maddr_48_bit_compressible(&ipv6->dst)) {
		NET_DBG("DAM_01 6 bytes: 2nd byte + last five bytes");

		/* 6 bytes: 2nd byte + last five bytes */
		IPHC[1] |= NET_6LO_IPHC_DAM_01;

		memcpy(&IPHC[offset], &ipv6->dst.s6_addr[1], 1);
		offset++;

		memcpy(&IPHC[offset], &ipv6->dst.s6_addr[11], 5);
		offset += 5;
	} else {
		NET_DBG("DAM_00 dst complete addr inlined");

		/* complete address IPHC[1] |= NET_6LO_IPHC_DAM_00 */
		memcpy(&IPHC[offset], &ipv6->dst.s6_addr[0], 16);
		offset += 16;
	}

	return offset;

}

static inline u8_t compress_da(struct net_ipv6_hdr *ipv6,
				  struct net_pkt *pkt,
				  struct net_buf *frag,
				  u8_t offset)
{
	/* If destination address is multicast */
	if (net_is_ipv6_addr_mcast(&ipv6->dst)) {
		return compress_da_mcast(ipv6, pkt, frag, offset);
	}

	/* If address is link-local prefix and padded with zeros */
	if (net_is_ipv6_ll_addr(&ipv6->dst) &&
	    net_6lo_ll_prefix_padded_with_zeros(&ipv6->dst)) {

		NET_DBG("Dst is ll_addr and padded with zeros");

		/* Following 64 bits are 0000:00ff:fe00:XXXX */
		if (net_6lo_addr_16_bit_compressible(&ipv6->dst)) {
			NET_DBG("DAM_10 dst addr 16 bit compressible");

			IPHC[1] |= NET_6LO_IPHC_DAM_10;

			memcpy(&IPHC[offset], &ipv6->dst.s6_addr[14], 2);
			offset += 2;
		} else {
			if (!net_pkt_ll_dst(pkt)) {
				NET_ERR("Invalid dst ll address");
				return 0;
			}

			if (net_ipv6_addr_based_on_ll(&ipv6->dst,
						      net_pkt_ll_dst(pkt))) {
				NET_DBG("DAM_11 dst addr fully elided");

				/* Address is fully elided */
				IPHC[1] |= NET_6LO_IPHC_DAM_11;
			} else {
				NET_DBG("DAM_01 remaining 64 bits are inlined");

				/* Remaining 64 bits are in-line */
				IPHC[1] |= NET_6LO_IPHC_DAM_01;

				memcpy(&IPHC[offset], &ipv6->dst.s6_addr[8], 8);
				offset += 8;
			}
		}
	} else {
		NET_DBG("DAM_00 dst full addr inlined");
		IPHC[1] |= NET_6LO_IPHC_DAM_00;

		memcpy(&IPHC[offset], &ipv6->dst.s6_addr[0], 16);
		offset += 16;
	}

	return offset;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
static inline u8_t compress_da_ctx(struct net_ipv6_hdr *ipv6,
				      struct net_pkt *pkt,
				      struct net_buf *frag,
				      u8_t offset,
				      struct net_6lo_context *dst)
{
	if (!dst) {
		return compress_da(ipv6, pkt, frag, offset);
	}

	IPHC[1] |= NET_6LO_IPHC_DAC_1;

	/* Following 64 bits are 0000:00ff:fe00:XXXX */
	if (net_6lo_addr_16_bit_compressible(&ipv6->dst)) {
		NET_DBG("DAM_10 dst addr 16 bit compressible");

		IPHC[1] |= NET_6LO_IPHC_DAM_10;

		memcpy(&IPHC[offset], &ipv6->dst.s6_addr[14], 2);
		offset += 2;
	} else {
		if (net_ipv6_addr_based_on_ll(&ipv6->dst,
					      net_pkt_ll_dst(pkt))) {
			NET_DBG("DAM_11 dst addr fully elided");

			/* Address is fully elided */
			IPHC[1] |= NET_6LO_IPHC_DAM_11;
		} else {
			NET_DBG("DAM_01 remaining 64 bits are inlined");

			/* Remaining 64 bits are in-line */
			IPHC[1] |= NET_6LO_IPHC_DAM_01;

			memcpy(&IPHC[offset], &ipv6->dst.s6_addr[8], 8);
			offset += 8;
		}
	}

	return offset;
}
#endif

/* Helper to compress Next header UDP */
static inline u8_t compress_nh_udp(struct net_udp_hdr *udp,
				      struct net_buf *frag, u8_t offset)
{
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

	if ((((htons(udp->src_port) >> 4) & 0xFFF) ==
	    NET_6LO_NHC_UDP_4_BIT_PORT) &&
	    (((htons(udp->dst_port) >> 4) & 0xFFF) ==
	    NET_6LO_NHC_UDP_4_BIT_PORT)) {

		NET_DBG("UDP ports src and dst 4 bits inlined");
		/** src: first 16 bits elided, next 4 bits inlined
		  * dst: first 16 bits elided, next 4 bits inlined
		  */
		IPHC[offset] |= NET_6LO_NHC_UDP_PORT_11;
		offset++;

		tmp = (u8_t)(htons(udp->src_port));
		tmp = tmp << 4;

		tmp |= (((u8_t)(htons(udp->dst_port))) & 0x0F);
		IPHC[offset++] = tmp;
	} else if (((htons(udp->dst_port) >> 8) & 0xFF) ==
		   NET_6LO_NHC_UDP_8_BIT_PORT) {

		NET_DBG("UDP ports src full, dst 8 bits inlined");
		/* dst: first 8 bits elided, next 8 bits inlined
		 * src: fully carried inline
		 */
		IPHC[offset] |= NET_6LO_NHC_UDP_PORT_01;
		offset++;

		memcpy(&IPHC[offset], &udp->src_port, 2);
		offset += 2;

		IPHC[offset++] = (u8_t)(htons(udp->dst_port));
	} else if (((htons(udp->src_port) >> 8) & 0xFF) ==
		    NET_6LO_NHC_UDP_8_BIT_PORT) {

		NET_DBG("UDP ports src 8bits, dst full inlined");
		/* src: first 8 bits elided, next 8 bits inlined
		 * dst: fully carried inline
		 */
		IPHC[offset] |= NET_6LO_NHC_UDP_PORT_10;
		offset++;

		IPHC[offset++] = (u8_t)(htons(udp->src_port));

		memcpy(&IPHC[offset], &udp->dst_port, 2);
		offset += 2;
	} else {
		NET_DBG("Can not compress ports, ports are inlined");

		/* can not compress ports, ports are inlined */
		offset++;
		memcpy(&IPHC[offset], &udp->src_port, 4);
		offset += 4;
	}

	/* All 16 bits of udp chksum are inlined, length is elided */
	memcpy(&IPHC[offset], &udp->chksum, 2);
	offset += 2;

	return offset;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
static inline bool is_src_and_dst_addr_ctx_based(struct net_ipv6_hdr *ipv6,
						 struct net_pkt *pkt,
						 struct net_buf *frag,
						 struct net_6lo_context **src,
						 struct net_6lo_context **dst)
{
	/* If compress flag is unset means use only in uncompression. */

	*src = get_6lo_context_by_addr(net_pkt_iface(pkt), &ipv6->src);
	if (*src && !((*src)->compress)) {
		*src = NULL;
	}

	*dst = get_6lo_context_by_addr(net_pkt_iface(pkt), &ipv6->dst);
	if (*dst && !((*dst)->compress)) {
		*dst = NULL;
	}

	if (!*src && !*dst) {
		return false;
	}

	NET_DBG("Context based compression");
	IPHC[1] |= NET_6LO_IPHC_CID_1;
	IPHC[2] = 0;

	if (*src) {
		NET_DBG("Src addr context cid %d", (*src)->cid);
		IPHC[2] = (*src)->cid << 4;
	}

	if (*dst) {
		NET_DBG("Dst addr context cid %d", (*dst)->cid);
		IPHC[2] |= (*dst)->cid;
	}

	return true;
}

#endif

/* RFC 6282 LOWPAN IPHC Encoding format (3.1)
 *  Base Format
 *   0                                       1
 *   0   1   2   3   4   5   6   7   8   9   0   1   2   3   4   5
 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 * | 0 | 1 | 1 |  TF   |NH | HLIM  |CID|SAC|  SAM  | M |DAC|  DAM  |
 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 */
static inline bool compress_IPHC_header(struct net_pkt *pkt,
					fragment_handler_t fragment)
{
#if defined(CONFIG_NET_6LO_CONTEXT)
	struct net_6lo_context *src = NULL;
	struct net_6lo_context *dst = NULL;
#endif
	struct net_ipv6_hdr *ipv6 = NET_IPV6_HDR(pkt);
	u8_t offset = 0;
	struct net_udp_hdr *udp;
	struct net_buf *frag;
	u8_t compressed;

	if (pkt->frags->len < NET_IPV6H_LEN) {
		NET_ERR("Invalid length %d, min %d",
			pkt->frags->len, NET_IPV6H_LEN);
		return false;
	}

	if (ipv6->nexthdr == IPPROTO_UDP &&
	    pkt->frags->len < NET_IPV6UDPH_LEN) {
		NET_ERR("Invalid length %d, min %d",
			pkt->frags->len, NET_IPV6UDPH_LEN);
		return false;
	}

	frag = net_pkt_get_frag(pkt, K_FOREVER);

	IPHC[offset++] = NET_6LO_DISPATCH_IPHC;
	IPHC[offset++] = 0;

#if defined(CONFIG_NET_6LO_CONTEXT)
	if (is_src_and_dst_addr_ctx_based(ipv6, pkt, frag, &src, &dst)) {
		offset++;
	}
#endif

	/* Compress Traffic class and Flow lablel */
	offset = compress_tfl(ipv6, frag, offset);

	/* Next Header */
	offset = compress_nh(ipv6, frag, offset);

	/* Hop limit */
	offset = compress_hoplimit(ipv6, frag, offset);

	/* Source Address Compression */
#if defined(CONFIG_NET_6LO_CONTEXT)
	offset = compress_sa_ctx(ipv6, pkt, frag, offset, src);
#else
	offset = compress_sa(ipv6, pkt, frag, offset);
#endif
	if (!offset) {
		net_pkt_frag_unref(frag);
		return false;
	}

	/* Destination Address Compression */
#if defined(CONFIG_NET_6LO_CONTEXT)
	offset = compress_da_ctx(ipv6, pkt, frag, offset, dst);
#else
	offset = compress_da(ipv6, pkt, frag, offset);
#endif

	if (!offset) {
		net_pkt_frag_unref(frag);
		return false;
	}

	compressed = NET_IPV6H_LEN;

	if (ipv6->nexthdr != IPPROTO_UDP) {
		NET_DBG("next header is not UDP (%u)", ipv6->nexthdr);
		goto end;
	}

	/* UDP header compression */
	udp = NET_UDP_HDR(pkt);
	IPHC[offset] = NET_6LO_NHC_UDP_BARE;
	offset = compress_nh_udp(udp, frag, offset);

	compressed += NET_UDPH_LEN;

end:
	net_buf_add(frag, offset);

	/* Copy the rest of the data to compressed fragment */
	memcpy(&IPHC[offset], pkt->frags->data + compressed,
	       pkt->frags->len - compressed);
	net_buf_add(frag, pkt->frags->len - compressed);

	/* Delete uncompressed(original) header fragment */
	net_pkt_frag_del(pkt, NULL, pkt->frags);

	/* Insert compressed header fragment */
	net_pkt_frag_insert(pkt, frag);

	/* Compact the fragments, so that gaps will be filled */
	net_pkt_compact(pkt);

	if (fragment) {
		return fragment(pkt, compressed - offset);
	}

	return true;
}

/* Helper to uncompress Traffic class and Flow label */
static inline u8_t uncompress_tfl(struct net_pkt *pkt,
				     struct net_ipv6_hdr *ipv6,
				     u8_t offset)
{
	u8_t tcl;

	/* Uncompress tcl and flow label */
	switch (CIPHC[0] & NET_6LO_IPHC_TF_11) {
	case NET_6LO_IPHC_TF_00:
		NET_DBG("ECN + DSCP + 4-bit Pad + Flow Label");

		tcl = CIPHC[offset++];
		tcl = (tcl >> 6) | (tcl << 2);

		ipv6->vtc |= ((tcl & 0xF0) >> 4);
		ipv6->tcflow = ((tcl & 0x0F) << 4) | (CIPHC[offset++] & 0x0F);

		memcpy(&ipv6->flow, &CIPHC[offset], 2);
		offset += 2;
		break;
	case NET_6LO_IPHC_TF_01:
		NET_DBG("ECN + 2-bit Pad + Flow Label, DSCP is elided");

		tcl = ((CIPHC[offset] & 0xF0) >> 6);
		ipv6->tcflow = ((tcl & 0x0F) << 4) | (CIPHC[offset++] & 0x0F);

		memcpy(&ipv6->flow, &CIPHC[offset], 2);
		offset += 2;
		break;
	case NET_6LO_IPHC_TF_10:
		NET_DBG("Flow label elided");

		tcl = CIPHC[offset];
		tcl = (tcl >> 6) | (tcl << 2);

		ipv6->vtc |= ((tcl & 0xF0) >> 4);
		ipv6->tcflow = (tcl & 0x0F) << 4;
		ipv6->flow = 0;
		offset++;
		break;
	case NET_6LO_IPHC_TF_11:
		NET_DBG("Tcl and Flow label elided");

		ipv6->tcflow = 0;
		ipv6->flow = 0;
		break;
	}

	return offset;
}

/* Helper to uncompress Hoplimit */
static inline u8_t uncompress_hoplimit(struct net_pkt *pkt,
					  struct net_ipv6_hdr *ipv6,
					  u8_t offset)
{
	switch (CIPHC[0] & NET_6LO_IPHC_HLIM255) {
	case NET_6LO_IPHC_HLIM:
		ipv6->hop_limit = CIPHC[offset++];
		break;
	case NET_6LO_IPHC_HLIM1:
		ipv6->hop_limit = 1;
		break;
	case NET_6LO_IPHC_HLIM64:
		ipv6->hop_limit = 64;
		break;
	case NET_6LO_IPHC_HLIM255:
		ipv6->hop_limit = 255;
		break;
	}

	return offset;
}

/* Helper to uncompress Source Address */
static inline u8_t uncompress_sa(struct net_pkt *pkt,
				    struct net_ipv6_hdr *ipv6,
				    u8_t offset)
{

	NET_DBG("SAC_0");

	switch (CIPHC[1] & NET_6LO_IPHC_SAM_11) {
	case NET_6LO_IPHC_SAM_00:
		NET_DBG("SAM_00 full src addr inlined");

		memcpy(ipv6->src.s6_addr, &CIPHC[offset], 16);
		offset += 16;
		break;
	case NET_6LO_IPHC_SAM_01:
		NET_DBG("SAM_01 last 64 bits are inlined");

		ipv6->src.s6_addr[0] = 0xFE;
		ipv6->src.s6_addr[1] = 0x80;

		memcpy(&ipv6->src.s6_addr[8], &CIPHC[offset], 8);
		offset += 8;
		break;
	case NET_6LO_IPHC_SAM_10:
		NET_DBG("SAM_10 src addr 16 bit compressed");

		ipv6->src.s6_addr[0] = 0xFE;
		ipv6->src.s6_addr[1] = 0x80;
		ipv6->src.s6_addr[11] = 0xFF;
		ipv6->src.s6_addr[12] = 0xFE;

		memcpy(&ipv6->src.s6_addr[14], &CIPHC[offset], 2);
		offset += 2;
		break;
	case NET_6LO_IPHC_SAM_11:
		NET_DBG("SAM_11 generate src addr from ll");

		net_ipv6_addr_create_iid(&ipv6->src, net_pkt_ll_src(pkt));
		break;
	}

	return offset;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
static inline u8_t uncompress_sa_ctx(struct net_pkt *pkt,
					struct net_ipv6_hdr *ipv6,
					u8_t offset,
					struct net_6lo_context *ctx)
{
	NET_DBG("SAC_1");

	switch (CIPHC[1] & NET_6LO_IPHC_SAM_11) {
	case NET_6LO_IPHC_SAM_00:
		NET_DBG("SAM_00 unspecified address");
		break;
	case NET_6LO_IPHC_SAM_01:
		NET_DBG("SAM_01 last 64 bits are inlined");

		/* First 8 bytes are from context */
		memcpy(&ipv6->src.s6_addr[0], &ctx->prefix.s6_addr[0], 8);

		/* And the rest are carried in-line*/
		memcpy(&ipv6->src.s6_addr[8], &CIPHC[offset], 8);
		offset += 8;

		break;
	case NET_6LO_IPHC_SAM_10:
		NET_DBG("SAM_10 src addr 16 bit compressed");

		/* First 8 bytes are from context */
		memcpy(&ipv6->src.s6_addr[0], &ctx->prefix.s6_addr[0], 8);

		ipv6->src.s6_addr[11] = 0xFF;
		ipv6->src.s6_addr[12] = 0xFE;

		/* And the rest are carried in-line */
		memcpy(&ipv6->src.s6_addr[14], &CIPHC[offset], 2);
		offset += 2;

		break;
	case NET_6LO_IPHC_SAM_11:
		NET_DBG("SAM_11 generate src addr from ll");
		/* RFC 6282, 3.1.1. If SAC = 1 and SAM = 11
		 * Derive addr using context information and
		 * the encapsulating header.
		 * (e.g., 802.15.4 or IPv6 source address).
		 */
		net_ipv6_addr_create_iid(&ipv6->src, net_pkt_ll_src(pkt));

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
					  u8_t offset)
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

		memcpy(&ipv6->dst.s6_addr[0], &CIPHC[offset], 16);
		offset += 16;
		break;
	case NET_6LO_IPHC_DAM_01:
		NET_DBG("DAM_01 2nd byte and last five bytes");

		ipv6->dst.s6_addr[0] = 0xFF;
		ipv6->dst.s6_addr[1] = CIPHC[offset++];

		memcpy(&ipv6->dst.s6_addr[11], &CIPHC[offset], 5);
		offset += 5;
		break;
	case NET_6LO_IPHC_DAM_10:
		NET_DBG("DAM_10 2nd byte and last three bytes");

		ipv6->dst.s6_addr[0] = 0xFF;
		ipv6->dst.s6_addr[1] = CIPHC[offset++];

		memcpy(&ipv6->dst.s6_addr[13], &CIPHC[offset], 3);
		offset += 3;
		break;
	case NET_6LO_IPHC_DAM_11:
		NET_DBG("DAM_11 8 bit compressed");

		ipv6->dst.s6_addr[0] = 0xFF;
		ipv6->dst.s6_addr[1] = 0x02;
		ipv6->dst.s6_addr[15] = CIPHC[offset++];
		break;
	}

	return offset;
}

/* Helper to uncompress Destination Address */
static inline u8_t uncompress_da(struct net_pkt *pkt,
				    struct net_ipv6_hdr *ipv6,
				    u8_t offset)
{
	NET_DBG("DAC_0");

	if (CIPHC[1] & NET_6LO_IPHC_M_1) {
		return uncompress_da_mcast(pkt, ipv6, offset);
	}

	switch (CIPHC[1] & NET_6LO_IPHC_DAM_11) {
	case NET_6LO_IPHC_DAM_00:
		NET_DBG("DAM_00 full dst addr inlined");

		memcpy(&ipv6->dst.s6_addr[0], &CIPHC[offset], 16);
		offset += 16;
		break;
	case NET_6LO_IPHC_DAM_01:
		NET_DBG("DAM_01 last 64 bits are inlined");

		ipv6->dst.s6_addr[0] = 0xFE;
		ipv6->dst.s6_addr[1] = 0x80;

		memcpy(&ipv6->dst.s6_addr[8], &CIPHC[offset], 8);
		offset += 8;
		break;
	case NET_6LO_IPHC_DAM_10:
		NET_DBG("DAM_10 dst addr 16 bit compressed");

		ipv6->dst.s6_addr[0] = 0xFE;
		ipv6->dst.s6_addr[1] = 0x80;
		ipv6->dst.s6_addr[11] = 0xFF;
		ipv6->dst.s6_addr[12] = 0xFE;

		memcpy(&ipv6->dst.s6_addr[14], &CIPHC[offset], 2);
		offset += 2;
		break;
	case NET_6LO_IPHC_DAM_11:
		NET_DBG("DAM_11 generate dst addr from ll");

		net_ipv6_addr_create_iid(&ipv6->dst, net_pkt_ll_dst(pkt));
		break;
	}

	return offset;
}

#if defined(CONFIG_NET_6LO_CONTEXT)
static inline u8_t uncompress_da_ctx(struct net_pkt *pkt,
					struct net_ipv6_hdr *ipv6,
					u8_t offset,
					struct net_6lo_context *ctx)
{
	NET_DBG("DAC_1");

	if (CIPHC[1] & NET_6LO_IPHC_M_1) {
		return uncompress_da_mcast(pkt, ipv6, offset);
	}

	switch (CIPHC[1] & NET_6LO_IPHC_DAM_11) {
	case NET_6LO_IPHC_DAM_01:
		NET_DBG("DAM_01 last 64 bits are inlined");

		/* First 8 bytes are from context */
		memcpy(&ipv6->dst.s6_addr[0], &ctx->prefix.s6_addr[0], 8);

		/* And the rest are carried in-line */
		memcpy(&ipv6->dst.s6_addr[8], &CIPHC[offset], 8);
		offset += 8;

		break;
	case NET_6LO_IPHC_DAM_10:
		NET_DBG("DAM_10 src addr 16 bit compressed");

		/* First 8 bytes are from context */
		memcpy(&ipv6->dst.s6_addr[0], &ctx->prefix.s6_addr[0], 8);

		ipv6->dst.s6_addr[11] = 0xFF;
		ipv6->dst.s6_addr[12] = 0xFE;

		/* And the restare carried in-line */
		memcpy(&ipv6->dst.s6_addr[14], &CIPHC[offset], 2);
		offset += 2;

		break;
	case NET_6LO_IPHC_DAM_11:
		NET_DBG("DAM_11 generate src addr from ll");

		/* RFC 6282, 3.1.1. If SAC = 1 and SAM = 11
		 * Derive addr using context information and
		 * the encapsulating header.
		 * (e.g., 802.15.4 or IPv6 source address).
		 */
		net_ipv6_addr_create_iid(&ipv6->dst, net_pkt_ll_dst(pkt));

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
					u8_t offset)
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

		memcpy(&udp->src_port, &CIPHC[offset], 2);
		offset += 2;

		memcpy(&udp->dst_port, &CIPHC[offset], 2);
		offset += 2;
		break;
	case NET_6LO_NHC_UDP_PORT_01:
		NET_DBG("src full, dst 8 bits inlined");

		memcpy(&udp->src_port, &CIPHC[offset], 2);
		offset += 2;

		udp->dst_port = htons(((u16_t)NET_6LO_NHC_UDP_8_BIT_PORT
				       << 8) | CIPHC[offset]);
		offset++;
		break;
	case NET_6LO_NHC_UDP_PORT_10:
		NET_DBG("src 8 bits, dst full inlined");

		udp->src_port = htons(((u16_t)NET_6LO_NHC_UDP_8_BIT_PORT
				       << 8) | CIPHC[offset]);
		offset++;

		memcpy(&udp->dst_port, &CIPHC[offset], 2);
		offset += 2;
		break;
	case NET_6LO_NHC_UDP_PORT_11:
		NET_DBG("src and dst 4 bits inlined");

		udp->src_port = htons((NET_6LO_NHC_UDP_4_BIT_PORT << 4) |
				      (CIPHC[offset] >> 4));

		udp->dst_port = htons((NET_6LO_NHC_UDP_4_BIT_PORT << 4) |
				      (CIPHC[offset] & 0x0F));
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

static inline bool uncompress_IPHC_header(struct net_pkt *pkt)
{
	struct net_udp_hdr *udp = NULL;
	u8_t offset = 2;
	u8_t chksum = 0;
	struct net_ipv6_hdr *ipv6;
	struct net_buf *frag;
	u16_t len;
#if defined(CONFIG_NET_6LO_CONTEXT)
	struct net_6lo_context *src = NULL;
	struct net_6lo_context *dst = NULL;
#endif

	if (CIPHC[1] & NET_6LO_IPHC_CID_1) {
#if defined(CONFIG_NET_6LO_CONTEXT)
		uncompress_cid(pkt, &src, &dst);
		offset++;
#else
		NET_WARN("Context based uncompression not enabled");
		return false;
#endif
	}

	frag = net_pkt_get_frag(pkt, NET_6LO_RX_PKT_TIMEOUT);
	if (!frag) {
		return false;
	}

	ipv6 = (struct net_ipv6_hdr *)(frag->data);

	/* Version is always 6 */
	ipv6->vtc = 0x60;
	net_pkt_set_ip_hdr_len(pkt, NET_IPV6H_LEN);

	/* Uncompress Traffic class and Flow label */
	offset = uncompress_tfl(pkt, ipv6, offset);

	if (!(CIPHC[0] & NET_6LO_IPHC_NH_1)) {
		ipv6->nexthdr = CIPHC[offset];
		offset++;
	}

	/* Uncompress Hoplimit */
	offset = uncompress_hoplimit(pkt, ipv6, offset);

	/* First set to zero and copy relevant bits */
	memset(&ipv6->src.s6_addr[0], 0, 16);
	memset(&ipv6->dst.s6_addr[0], 0, 16);

	/* Uncompress Source Address */
#if defined(CONFIG_NET_6LO_CONTEXT)
	if (CIPHC[1] & NET_6LO_IPHC_SAC_1) {
		if (!src) {
			NET_ERR("SAC is set but src context doesn't exists");
			goto fail;
		}

		offset = uncompress_sa_ctx(pkt, ipv6, offset, src);
	} else {
		offset = uncompress_sa(pkt, ipv6, offset);
	}
#else
	offset = uncompress_sa(pkt, ipv6, offset);
#endif

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

		offset = uncompress_da_ctx(pkt, ipv6, offset, dst);
	} else {
		offset = uncompress_da(pkt, ipv6, offset);
	}
#else
	offset = uncompress_da(pkt, ipv6, offset);
#endif

	net_buf_add(frag, NET_IPV6H_LEN);

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
	ipv6->nexthdr = IPPROTO_UDP;

	udp = (struct net_udp_hdr *)(frag->data + NET_IPV6H_LEN);
	chksum = CIPHC[offset] & NET_6LO_NHC_UDP_CHKSUM_1;
	offset = uncompress_nh_udp(pkt, udp, offset);

	if (!chksum) {
		memcpy(&udp->chksum, &CIPHC[offset], 2);
		offset += 2;
	}

	net_buf_add(frag, NET_UDPH_LEN);

end:
	/* Move the data to beginning, no need for headers now */
	NET_DBG("Removing %u bytes of compressed hdr", offset);
	memmove(pkt->frags->data, pkt->frags->data + offset,
		pkt->frags->len - offset);
	pkt->frags->len -= offset;

	/* Copying ll part, if any */
	if (net_pkt_ll_reserve(pkt)) {
		memcpy(frag->data - net_pkt_ll_reserve(pkt),
		       net_pkt_ll(pkt), net_pkt_ll_reserve(pkt));
	}

	/* Insert the fragment (this one holds uncompressed headers) */
	net_pkt_frag_insert(pkt, frag);
	net_pkt_compact(pkt);

	/* Set IPv6 header and UDP (if next header is) length */
	len = net_pkt_get_len(pkt) - NET_IPV6H_LEN;
	ipv6->len[0] = len >> 8;
	ipv6->len[1] = (u8_t)len;

	if (ipv6->nexthdr == IPPROTO_UDP && udp) {
		udp->len = htons(len);

		if (chksum) {
			udp->chksum = ~net_calc_chksum_udp(pkt);
		}
	}

	return true;

fail:
	net_pkt_frag_unref(frag);
	return false;
}

/* Adds IPv6 dispatch as first byte and adjust fragments  */
static inline bool compress_ipv6_header(struct net_pkt *pkt,
					fragment_handler_t fragment)
{
	struct net_buf *frag;

	frag = net_pkt_get_frag(pkt, K_FOREVER);

	frag->data[0] = NET_6LO_DISPATCH_IPV6;
	net_buf_add(frag, 1);

	net_pkt_frag_insert(pkt, frag);

	/* Compact the fragments, so that gaps will be filled */
	net_pkt_compact(pkt);

	if (fragment) {
		return fragment(pkt, -1);
	}

	return true;
}

static inline bool uncompress_ipv6_header(struct net_pkt *pkt)
{
	struct net_buf *frag = pkt->frags;

	/* Pull off IPv6 dispatch header and adjust data and length */
	memmove(frag->data, frag->data + 1, frag->len - 1);
	frag->len -= 1;

	return true;
}

bool net_6lo_compress(struct net_pkt *pkt, bool iphc,
		      fragment_handler_t fragment)
{
	if (iphc) {
		return compress_IPHC_header(pkt, fragment);
	} else {
		return compress_ipv6_header(pkt, fragment);
	}
}

bool net_6lo_uncompress(struct net_pkt *pkt)
{
	NET_ASSERT(pkt && pkt->frags);

	if ((pkt->frags->data[0] & NET_6LO_DISPATCH_IPHC) ==
	    NET_6LO_DISPATCH_IPHC) {
		/* Uncompress IPHC header */
		return uncompress_IPHC_header(pkt);

	} else if ((pkt->frags->data[0] & NET_6LO_DISPATCH_IPV6) ==
		   NET_6LO_DISPATCH_IPV6) {
		/* Uncompress IPv6 header, it has only IPv6 dispatch in the
		 * beginning */
		return uncompress_ipv6_header(pkt);
	}

	NET_DBG("pkt %p is not compressed", pkt);

	return true;
}
