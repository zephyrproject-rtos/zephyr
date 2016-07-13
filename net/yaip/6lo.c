/** @file
 * @brief 6lopan related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(CONFIG_NET_6LO_DEBUG)
#define SYS_LOG_DOMAIN "net/6lo"
#define NET_DEBUG 1
#endif

#include <errno.h>
#include <net/net_core.h>
#include <net/nbuf.h>
#include <net/net_if.h>
#include <net/net_stats.h>

#include "net_private.h"
#include "6lo.h"
#include "6lo_private.h"

static inline bool net_6lo_ll_prefix_padded_with_zeros(struct in6_addr *addr)
{
	return ((addr->s6_addr[2] == 0x00) &&
		 (addr->s6_addr[3] == 0x00) &&
		 (addr->s6_addr[4] == 0x00) &&
		 (addr->s6_addr[5] == 0x00) &&
		 (addr->s6_addr[6] == 0x00) &&
		 (addr->s6_addr[7] == 0x00));

}

static inline bool net_6lo_addr_16_bit_compressible(struct in6_addr *addr)
{
	return ((addr->s6_addr[8] == 0x00) &&
		 (addr->s6_addr[9] == 0x00) &&
		 (addr->s6_addr[10] == 0x00) &&
		 (addr->s6_addr[11] == 0xFF) &&
		 (addr->s6_addr[12] == 0xFE) &&
		 (addr->s6_addr[13] == 0x00));
}

static inline bool net_6lo_maddr_8_bit_compressible(struct in6_addr *addr)
{
	return ((addr->s6_addr[1] == 0x02) &&
		 (addr->s6_addr16[1] == 0x00) &&
		 (addr->s6_addr32[1] == 0x00) &&
		 (addr->s6_addr32[2] == 0x00) &&
		 (addr->s6_addr[14] == 0x00));
}

static inline bool net_6lo_maddr_32_bit_compressible(struct in6_addr *addr)
{
	return ((addr->s6_addr32[1] == 0x00) &&
		 (addr->s6_addr32[2] == 0x00) &&
		 (addr->s6_addr[12] == 0x00));
}

static inline bool net_6lo_maddr_48_bit_compressible(struct in6_addr *addr)
{
	return ((addr->s6_addr32[1] == 0x00) &&
		 (addr->s6_addr16[4] == 0x00) &&
		 (addr->s6_addr[10] == 0x00));
}

/**
  * RFC 6282 LOWPAN IPHC Encoding format (3.1)
  *  Base Format
  *   0                                       1
  *   0   1   2   3   4   5   6   7   8   9   0   1   2   3   4   5
  * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  * | 0 | 1 | 1 |  TF   |NH | HLIM  |CID|SAC|  SAM  | M |DAC|  DAM  |
  * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  *
  */

/** TODO: context based (src) address compression
  * Unicast-Prefix based IPv6 Multicast(dst) address compression
  * context based (dst) address compression
  * Mesh header compression
  */
static inline bool compress_IPHC_header(struct net_buf *buf,
					fragment_handler_t fragment)
{
	struct net_ipv6_hdr *ipv6 = NET_IPV6_BUF(buf);
	uint8_t offset = 0;
	struct net_udp_hdr *udp;
	struct net_buf *frag;
	uint8_t compressed;
	uint8_t tcl;
	uint8_t tmp;

	if (buf->frags->len < NET_IPV6H_LEN) {
		NET_DBG("Invalid length %d, min %d",
			 buf->frags->len, NET_IPV6H_LEN);
		return false;
	}

	if (ipv6->nexthdr == IPPROTO_UDP &&
	    buf->frags->len < NET_IPV6UDPH_LEN) {
		NET_DBG("Invalid length %d, min %d",
			 buf->frags->len, NET_IPV6UDPH_LEN);
		return false;
	}

	frag = net_nbuf_get_reserve_data(net_nbuf_ll_reserve(buf));
	if (!frag) {
		return false;
	}

	IPHC[offset++] = NET_6LO_DISPATCH_IPHC;
	IPHC[offset++] = 0;

	/** +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
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

	/* Next header */
	if (ipv6->nexthdr == IPPROTO_UDP) {
		IPHC[0] |= NET_6LO_IPHC_NH_1;
	} else {
		IPHC[offset++] = ipv6->nexthdr;
	}

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

	/* Source Address Compression */
	/* If address is link-local prefix and padded with zeros */
	if (net_is_ipv6_ll_addr(&ipv6->src) &&
	    net_6lo_ll_prefix_padded_with_zeros(&ipv6->src)) {

		NET_DBG("src is ll_addr and prefix padded with zeros");

		/* and following 64 bits are 0000:00ff:fe00:XXXX */
		if (net_6lo_addr_16_bit_compressible(&ipv6->src)) {
			NET_DBG("src addr 16 bit compressible");

			IPHC[1] |= NET_6LO_IPHC_SAM_10;

			memcpy(&IPHC[offset], &ipv6->src.s6_addr[14], 2);
			offset += 2;
		} else {

			if (net_ipv6_addr_based_on_ll(&ipv6->src,
			    net_if_get_link_addr(net_nbuf_iface(buf)))) {
				NET_DBG("src address is fully elided");

				/* Address is fully elided */
				IPHC[1] |= NET_6LO_IPHC_SAM_11;
			} else {
				NET_DBG("src remaining 64 bits are inlined");

				/* remaining 64 bits are in-line */
				IPHC[1] |= NET_6LO_IPHC_SAM_01;

				memcpy(&IPHC[offset], &ipv6->src.s6_addr[8], 8);
				offset += 8;
			}
		}
	} else if (net_is_ipv6_addr_unspecified(&ipv6->src)) {
		NET_DBG("src is unspecified address");

		/* Unspecified IPv6 src address */
		IPHC[1] |= NET_6LO_IPHC_SAC_1;
		IPHC[1] |= NET_6LO_IPHC_SAM_00;

	} else {
		NET_DBG("full address is carried in-line");

		/* full address is carried in-line */
		IPHC[1] |= NET_6LO_IPHC_SAM_00;

		memcpy(&IPHC[offset], ipv6->src.s6_addr, 16);
		offset += 16;
	}

	/* if destination address is multicast */
	if (net_is_ipv6_addr_mcast(&ipv6->dst)) {

		IPHC[1] |= NET_6LO_IPHC_M_1;

		NET_DBG("dst is mcast");

		if (net_6lo_maddr_8_bit_compressible(&ipv6->dst)) {
			NET_DBG("sicslowpan maddr 8 bit compressible");

			/* last byte */
			IPHC[1] |= NET_6LO_IPHC_DAM_11;

			memcpy(&IPHC[offset], &ipv6->dst.s6_addr[15], 1);
			offset++;
		} else if (net_6lo_maddr_32_bit_compressible(&ipv6->dst)) {
			NET_DBG("4 bytes: 2nd byte + last three bytes");

			/* 4 bytes: 2nd byte + last three bytes */
			IPHC[1] |= NET_6LO_IPHC_DAM_10;

			memcpy(&IPHC[offset], &ipv6->dst.s6_addr[1], 1);
			offset++;

			memcpy(&IPHC[offset], &ipv6->dst.s6_addr[13], 3);
			offset += 3;
		} else if (net_6lo_maddr_48_bit_compressible(&ipv6->dst)) {
			NET_DBG("6 bytes: 2nd byte + last five bytes");

			/* 6 bytes: 2nd byte + last five bytes */
			IPHC[1] |= NET_6LO_IPHC_DAM_01;

			memcpy(&IPHC[offset], &ipv6->dst.s6_addr[1], 1);
			offset++;

			memcpy(&IPHC[offset], &ipv6->dst.s6_addr[11], 5);
			offset += 5;
		} else {
			NET_DBG("dst complete address inlined");

			/* complete address IPHC[1] |= NET_6LO_IPHC_DAM_00 */
			memcpy(&IPHC[offset], &ipv6->dst.s6_addr[0], 16);
			offset += 16;
		}
	} else if (net_is_ipv6_ll_addr(&ipv6->dst) &&
		   net_6lo_ll_prefix_padded_with_zeros(&ipv6->dst)) {

		NET_DBG("dst is ll_addr and padded with zeros");

		/* and following 64 bits are 0000:00ff:fe00:XXXX */
		if (net_6lo_addr_16_bit_compressible(&ipv6->dst)) {
			NET_DBG("dst sicslowpan addr 16 bit compressible");

			IPHC[1] |= NET_6LO_IPHC_DAM_10;

			memcpy(&IPHC[offset], &ipv6->dst.s6_addr[14], 2);
			offset += 2;
		} else {
			if (net_ipv6_addr_based_on_ll(&ipv6->dst,
				net_if_get_link_addr(net_nbuf_iface(buf)))) {

				NET_DBG("address is fully elided");

				/* Address is fully elided */
				IPHC[1] |= NET_6LO_IPHC_DAM_11;
			} else {
				NET_DBG("remaining 64 bits are inlined");

				/* remaining 64 bits are in-line */
				IPHC[1] |= NET_6LO_IPHC_DAM_01;

				memcpy(&IPHC[offset], &ipv6->dst.s6_addr[8], 8);
				offset += 8;
			}
		}
	} else {
		NET_DBG("dst complete address inlined");

		/* complete address IPHC[1] |= NET_6LO_IPHC_DAM_00 */
		memcpy(&IPHC[offset], &ipv6->dst.s6_addr[0], 16);
		offset += 16;
	}

	compressed = NET_IPV6H_LEN;

	if (ipv6->nexthdr != IPPROTO_UDP) {
		NET_DBG("next header UDP");
		goto end;
	}

	/** 4.3.3 UDP LOWPAN_NHC Format
	  *   0   1   2   3   4   5   6   7
	  * +---+---+---+---+---+---+---+---+
	  * | 1 | 1 | 1 | 1 | 0 | C |   P   |
	  * +---+---+---+---+---+---+---+---+
	  */

	/** Port compression
	  * 00:  All 16 bits for src and dst are inlined.
	  * 01:  All 16 bits for src port inlined. First 8 bits of dst port is
	  *      0xf0 and elided.  The remaining 8 bits of dst port inlined.
	  * 10:  First 8 bits of src port 0xf0 and elided. The remaining 8 bits
	  *      of src port inlined. All 16 bits of dst port inlined.
	  * 11:  First 12 bits of both src and dst are 0xf0b and elided. The
	  *      remaining 4 bits for each are inlined.
	  */

	udp = NET_UDP_BUF(buf);
	IPHC[offset] = NET_6LO_NHC_UDP_BARE;

	if ((((htons(udp->src_port) >> 4) & 0xFFF) ==
	    NET_6LO_NHC_UDP_4_BIT_PORT) &&
	    (((htons(udp->dst_port) >> 4) & 0xFFF) ==
	    NET_6LO_NHC_UDP_4_BIT_PORT)) {

		NET_DBG("udp ports src and dst 4 bits inlined");
		/** src: first 16 bits elided, next 4 bits inlined
		  * dst: first 16 bits elided, next 4 bits inlined
		  */
		IPHC[offset] |= NET_6LO_NHC_UDP_PORT_11;
		offset++;

		tmp = (uint8_t)(htons(udp->src_port));
		tmp = tmp << 4;

		tmp |= (((uint8_t)(htons(udp->dst_port))) & 0x0F);
		IPHC[offset++] = tmp;
	} else if (((htons(udp->dst_port) >> 8) & 0xFF) ==
		   NET_6LO_NHC_UDP_8_BIT_PORT) {

		NET_DBG("udp ports src full, dst 8 bits inlined");
		/** dst: first 8 bits elided, next 8 bits inlined
		  * src: fully carried inline
		  */
		IPHC[offset] |= NET_6LO_NHC_UDP_PORT_01;
		offset++;

		memcpy(&IPHC[offset], &udp->src_port, 2);
		offset += 2;

		IPHC[offset++] = (uint8_t)(htons(udp->dst_port));
	} else if (((htons(udp->src_port) >> 8) & 0xFF) ==
		    NET_6LO_NHC_UDP_8_BIT_PORT) {

		NET_DBG("udp ports src 8bits, dst full inlined");
		/** src: first 8 bits elided, next 8 bits inlined
		  * dst: fully carried inline
		  */
		IPHC[offset] |= NET_6LO_NHC_UDP_PORT_10;
		offset++;

		IPHC[offset++] = (uint8_t)(htons(udp->src_port));

		memcpy(&IPHC[offset], &udp->dst_port, 2);
		offset += 2;
	} else {
		NET_DBG("can not compress ports, ports are inlined");

		/* can not compress ports, ports are inlined */
		offset++;
		memcpy(&IPHC[offset], &udp->src_port, 4);
		offset += 4;
	}

	/* All 16 bits of udp chksum are inlined, length is elided */
	memcpy(&IPHC[offset], &udp->chksum, 2);
	offset += 2;
	compressed += NET_UDPH_LEN;

end:
	net_buf_add(frag, offset);
	/* copy the rest of the data to compressed fragment */
	memcpy(&IPHC[offset], buf->frags->data + compressed,
	       buf->frags->len - compressed);
	net_buf_add(frag, buf->frags->len - compressed);

	/* Copying ll part, if any */
	if (net_nbuf_ll_reserve(buf)) {
		memcpy(IPHC - net_nbuf_ll_reserve(buf),
		       net_nbuf_ll(buf), net_nbuf_ll_reserve(buf));
	}

	/* delete the uncompressed(original) fragment */
	net_buf_frag_del(buf, buf->frags);
	net_buf_frag_insert(buf, frag);

	/* compact the fragments, so that gaps will be filled */
	net_nbuf_compact(buf->frags);

	if (fragment) {
		return fragment(buf, compressed - offset);
	}

	return true;
}

/* TODO: context based uncompression not supported */
static inline bool uncompress_IPHC_header(struct net_buf *buf)
{
	struct net_udp_hdr *udp = NULL;
	uint8_t offset = 2;
	struct net_ipv6_hdr *ipv6;
	struct net_buf *frag;
	uint8_t chksum;
	uint16_t len;
	uint8_t tcl;

	if (CIPHC[1] & NET_6LO_IPHC_CID_1) {
		/* if it is supported increase offset to +1 */
		return false;
	}

	frag = net_nbuf_get_reserve_data(net_nbuf_ll_reserve(buf));
	if (!frag) {
		return false;
	}

	ipv6 = (struct net_ipv6_hdr *)(frag->data);

	/* Version is always 6 */
	ipv6->vtc = 0x60;
	net_nbuf_set_ip_hdr_len(buf, NET_IPV6H_LEN);

	/* uncompress tcl and flow label */
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

	if (!(CIPHC[0] & NET_6LO_IPHC_NH_1)) {
		ipv6->nexthdr = CIPHC[offset];
		offset++;
	}

	switch (CIPHC[0] & NET_6LO_IPHC_HLIM255) {
	case NET_6LO_IPHC_HLIM:
		ipv6->hop_limit = 0;
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
	default:
		ipv6->hop_limit = CIPHC[offset++];
		break;
	}

	/* First set to zero and copy relevant bits */
	memset(&ipv6->src.s6_addr[0], 0, 16);
	memset(&ipv6->dst.s6_addr[0], 0, 16);

	if (CIPHC[1] & NET_6LO_IPHC_SAC_1) {
		NET_DBG("SAC is set");
		if (!(CIPHC[1] & NET_6LO_IPHC_SAM_00)) {
			NET_DBG("SAM_00 unspecified address");
		} else {
			/* TODO: context based uncompression */
			goto fail;
		}
	} else {
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

			net_ipv6_addr_create_iid(&ipv6->src,
						 net_nbuf_ll_src(buf));
			break;
		}
	}

	if (CIPHC[1] & NET_6LO_IPHC_DAC_1) {
		/* TODO: context based uncompression */
		goto fail;
	}

	if (CIPHC[1] & NET_6LO_IPHC_M_1) {
	/** If M=1 and DAC=0:
	  * 00: 128 bits, The full address is carried in-line.
	  * 01:  48 bits, The address takes the form ffXX::00XX:XXXX:XXXX.
	  * 10:  32 bits, The address takes the form ffXX::00XX:XXXX.
	  * 11:   8 bits, The address takes the form ff02::00XX.
	  */
		NET_DBG("dst is multicast");
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
	} else {
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

			net_ipv6_addr_create_iid(&ipv6->dst,
						 net_nbuf_ll_dst(buf));
			break;
		}
	}

	net_buf_add(frag, NET_IPV6H_LEN);

	if (!(CIPHC[0] & NET_6LO_IPHC_NH_1)) {
		goto end;
	}

	if ((CIPHC[offset] & 0xF0) != NET_6LO_NHC_UDP_BARE) {
		/*doesn't support other NH uncompression */
		goto fail;
	}

	/** Port uncompression
	  * 00:  All 16 bits for src and dst are inlined
	  * 01: src, 16 bits are lined, dst(0xf0) 8 bits are inlined
	  * 10: dst, 16 bits are lined, src(0xf0) 8 bits are inlined
	  * 11: src, dst (0xf0b) 4 bits are inlined
	  */
	ipv6->nexthdr = IPPROTO_UDP;
	udp = (struct net_udp_hdr *)(frag->data + NET_IPV6H_LEN);

	chksum = CIPHC[offset] & NET_6LO_NHC_UDP_CHKSUM_1;

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

		udp->dst_port = htons(((uint16_t)NET_6LO_NHC_UDP_8_BIT_PORT
				       << 8) | CIPHC[offset]);
		offset++;
		break;
	case NET_6LO_NHC_UDP_PORT_10:
		NET_DBG("src 8 bits, dst full inlined");

		udp->src_port = htons(((uint16_t)NET_6LO_NHC_UDP_8_BIT_PORT
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

	if (chksum) {
		/* TODO: calculate checksum */
		goto fail;
	}

	memcpy(&udp->chksum, &CIPHC[offset], 2);
	offset += 2;
	net_buf_add(frag, NET_UDPH_LEN);

end:
	/* move the data to beginning, no need for headers now */
	memmove(buf->frags->data, buf->frags->data + offset,
		buf->frags->len - offset);
	buf->frags->len -= offset;

	/* Copying ll part, if any */
	if (net_nbuf_ll_reserve(buf)) {
		memcpy(frag->data - net_nbuf_ll_reserve(buf),
		       net_nbuf_ll(buf), net_nbuf_ll_reserve(buf));
	}

	/* insert the fragment (this one holds uncompressed headers) */
	net_buf_frag_insert(buf, frag);
	net_nbuf_compact(buf->frags);

	/* set IPv6 header and UDP (if next header is) length */
	len = net_buf_frags_len(buf) - NET_IPV6H_LEN;
	ipv6->len[0] = len >> 8;
	ipv6->len[1] = (uint8_t)len;

	if (ipv6->nexthdr == IPPROTO_UDP) {
		udp->len = htons(len);
	}

	return true;

fail:
	net_nbuf_unref(frag);
	return false;
}

/* Adds IPv6 dispatch as first byte and adjust fragments  */
static inline bool compress_ipv6_header(struct net_buf *buf,
					fragment_handler_t fragment)
{
	struct net_buf *frag;

	frag = net_nbuf_get_reserve_data(net_nbuf_ll_reserve(buf));
	if (!frag) {
		return false;
	}

	frag->data[0] = NET_6LO_DISPATCH_IPV6;
	net_buf_add(frag, 1);

	net_buf_frag_insert(buf, frag);

	/* compact the fragments, so that gaps will be filled */
	buf->frags = net_nbuf_compact(buf->frags);

	if (fragment) {
		return fragment(buf, -1);
	}

	return true;
}

static inline bool uncompress_ipv6_header(struct net_buf *buf)
{
	struct net_buf *frag = buf->frags;

	/* Pull off IPv6 dispatch header and adjust data and length */
	memmove(frag->data, frag->data + 1, frag->len - 1);
	frag->len -= 1;

	return true;
}

bool net_6lo_compress(struct net_buf *buf, bool iphc,
		      fragment_handler_t fragment)
{
	if (iphc) {
		return compress_IPHC_header(buf, fragment);
	} else {
		return compress_ipv6_header(buf, fragment);
	}
}

bool net_6lo_uncompress(struct net_buf *buf)
{
	NET_ASSERT(buf && buf->frags);

	if ((buf->frags->data[0] & NET_6LO_DISPATCH_IPHC) ==
	    NET_6LO_DISPATCH_IPHC) {
		/* uncompress IPHC header */
		return uncompress_IPHC_header(buf);

	} else if ((buf->frags->data[0] & NET_6LO_DISPATCH_IPV6) ==
		   NET_6LO_DISPATCH_IPV6) {
		/* uncompress IPv6 header, it has only IPv6 dispatch in the
		 * beginning */
		return uncompress_ipv6_header(buf);
	}

	NET_DBG("Buf is not compressed");

	return true;
}
