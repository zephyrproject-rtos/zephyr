/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief 802.15.4 6LoWPAN adaptation layer
 *
 * This is not to be included by the application.
 */

#ifndef __NET_IEEE802154_6LO_H__
#define __NET_IEEE802154_6LO_H__

#include <zephyr/net/net_pkt.h>
#include <zephyr/types.h>

struct ieee802154_6lo_fragment_ctx {
	struct net_buf *buf; /* current original fragment pointer */
	uint8_t *pos;	     /* current position in buf */
	uint16_t pkt_size;   /* overall datagram size */
	uint16_t processed;  /* in bytes */
	uint8_t hdr_diff;    /* 6lo header size reduction due to compression in bytes */
	uint8_t offset;	     /* in multiples of 8 bytes */
};

/**
 *  @brief Decode and reassemble 6LoWPAN packets to IPv6 as per RFC 6282
 *
 *  @details Decompress and (if fragmented) reassemble 6LoWPAN packets for
 *  received over 802.15.4.
 *
 *  @param iface A valid pointer on the network interface the package was
 *               received from
 *  @param pkt A valid pointer on a 6LoWPAN compressed packet to receive
 *
 *  @return NET_CONTINUE decoding successful, pkt is decompressed
 *          NET_OK waiting for other fragments,
 *          NET_DROP invalid fragment.
 */
enum net_verdict ieee802154_6lo_decode_pkt(struct net_if *iface, struct net_pkt *pkt);

/**
 *  @brief Encode IPv6 packates to 6LoWPAN 802.15.4 as per RFC 6282
 *
 *  @details Compress IPv6 packet for transmission over 802.15.4 and
 *           check whether additional fragmentation is needed.
 *
 *  @param iface A valid pointer on the network interface the package is
 *               sent to
 *  @param pkt A valid pointer on a non-compressed IPv6 packet
 *  @param f_ctx A valid pointer on a fragmentation context.
 *  @param ll_hdr_len The size of the link layer header plus (if security
 *         is enabled) the authentication tag
 *
 *  @return 1 if additional 6LoWPAN fragmentation is needed, 0 if no
 *          fragmentation is needed, negative value on error
 */
int ieee802154_6lo_encode_pkt(struct net_if *iface, struct net_pkt *pkt,
			      struct ieee802154_6lo_fragment_ctx *f_ctx, uint8_t ll_hdr_len);

#endif /* __NET_IEEE802154_6LO_H__ */
