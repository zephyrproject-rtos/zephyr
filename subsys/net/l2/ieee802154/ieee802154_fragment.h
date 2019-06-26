/** @file
 @brief 802.15.4 fragment handler

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __NET_IEEE802154_FRAGMENT_H__
#define __NET_IEEE802154_FRAGMENT_H__

#include <sys/slist.h>
#include <zephyr/types.h>

#include <net/net_pkt.h>

#include "ieee802154_frame.h"

struct ieee802154_fragment_ctx {
	struct net_buf *buf;
	u8_t *pos;
	u16_t pkt_size;
	u16_t processed;
	u8_t hdr_diff;
	u8_t offset;
};

static inline bool ieee802154_fragment_is_needed(struct net_pkt *pkt,
						 u8_t ll_hdr_size)
{
	return (net_pkt_get_len(pkt) + ll_hdr_size >
			IEEE802154_MTU - IEEE802154_MFR_LENGTH);
}

static inline
void ieee802154_fragment_ctx_init(struct ieee802154_fragment_ctx *ctx,
				  struct net_pkt *pkt, u16_t hdr_diff,
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
 *  @param Pointer to valid fragmentation context
 *  @param Pointer to valid buffer where to write the fragment
 *  @param bool true for IPHC compression, false for IPv6 dispatch header
 *
 *  @return True in case of success, false otherwise
 */
#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
void ieee802154_fragment(struct ieee802154_fragment_ctx *ctx,
			 struct net_buf *frame_buf, bool iphc);
#else
#define ieee802154_fragment(...)
#endif

/**
 *  @brief Reassemble 802.15.4 fragments as per RFC 6282
 *
 *  @details If the data does not fit into single fragment whole IPv6 packet
 *  comes in number of fragments. This function will reassemble them all as
 *  per data tag, data offset and data size. First packet is uncompressed
 *  immediately after reception.
 *
 *  @param Pointer to network fragment, which gets updated to full reassembled
 *         packet when verdict is NET_CONTINUE
 *
 *  @return NET_CONTINUE reassembly done, pkt is complete
 *          NET_OK waiting for other fragments,
 *          NET_DROP invalid fragment.
 */
#ifdef CONFIG_NET_L2_IEEE802154_FRAGMENT
enum net_verdict ieee802154_reassemble(struct net_pkt *pkt);
#else
#define ieee802154_reassemble(...)
#endif /* CONFIG_NET_L2_IEEE802154_FRAGMENT */

#endif /* __NET_IEEE802154_FRAGMENT_H__ */
