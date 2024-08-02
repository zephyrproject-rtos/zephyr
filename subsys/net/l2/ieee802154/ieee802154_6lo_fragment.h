/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief 802.15.4 6LoWPAN fragment handler
 *
 * This is not to be included by the application.
 */

#ifndef __NET_IEEE802154_6LO_FRAGMENT_H__
#define __NET_IEEE802154_6LO_FRAGMENT_H__

#include "ieee802154_6lo.h"
#include "ieee802154_frame.h"

#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/slist.h>
#include <zephyr/types.h>

static inline bool ieee802154_6lo_requires_fragmentation(struct net_pkt *pkt, uint8_t ll_hdr_len,
							 uint8_t authtag_len)
{
	return (ll_hdr_len + net_pkt_get_len(pkt) + authtag_len > IEEE802154_MTU);
}

static inline void ieee802154_6lo_fragment_ctx_init(struct ieee802154_6lo_fragment_ctx *ctx,
						    struct net_pkt *pkt, uint16_t hdr_diff,
						    bool iphc)
{
	ctx->buf = pkt->buffer;
	ctx->pos = ctx->buf->data;
	ctx->hdr_diff = hdr_diff;
	ctx->pkt_size = net_pkt_get_len(pkt) + (iphc ? hdr_diff : -1);
	ctx->offset = 0U;
	ctx->processed = 0U;
}

/**
 *  @brief Fragment IPv6 packet as per RFC 6282
 *
 *  @details After IPv6 compression, transmission of IPv6 over 802.15.4
 *  needs to be fragmented. Every fragment will have fragmentation header
 *  data size, data offset, data tag and payload.
 *
 *  @param ctx Pointer to valid fragmentation context
 *  @param frame_buf Pointer to valid buffer where to write the fragment
 *  @param ipch bool true for IPHC compression, false for IPv6 dispatch header
 *
 *  @return pointer to the next buffer to be processed or NULL if no more
 *          buffers need processing
 */
struct net_buf *ieee802154_6lo_fragment(struct ieee802154_6lo_fragment_ctx *ctx,
					struct net_buf *frame_buf, bool iphc);

/**
 *  @brief Reassemble 802.15.4 fragments as per RFC 6282
 *
 *  @details If the data does not fit into single fragment whole IPv6 packet
 *  comes in number of fragments. This function will reassemble them all as
 *  per data tag, data offset and data size. First packet is uncompressed
 *  immediately after reception.
 *
 *  @param pkt Pointer to network fragment, which gets updated to full reassembled
 *         packet when verdict is NET_CONTINUE
 *
 *  @return NET_CONTINUE reassembly done, pkt is complete
 *          NET_OK waiting for other fragments,
 *          NET_DROP invalid fragment.
 */
enum net_verdict ieee802154_6lo_reassemble(struct net_pkt *pkt);

#endif /* __NET_IEEE802154_6LO_FRAGMENT_H__ */
