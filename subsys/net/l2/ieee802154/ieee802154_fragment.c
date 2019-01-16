/** @file
 * @brief 802.15.4 fragment related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154_fragment,
		    CONFIG_NET_L2_IEEE802154_LOG_LEVEL);

#include <errno.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_stats.h>
#include <net/udp.h>

#include "ieee802154_fragment.h"

#include "net_private.h"
#include "6lo.h"
#include "6lo_private.h"

#define BUF_TIMEOUT K_MSEC(50)

#define FRAG_REASSEMBLY_TIMEOUT \
	K_SECONDS(CONFIG_NET_L2_IEEE802154_REASSEMBLY_TIMEOUT)
#define REASS_CACHE_SIZE CONFIG_NET_L2_IEEE802154_FRAGMENT_REASS_CACHE_SIZE

static u16_t datagram_tag;

/**
 *  Reassemble cache : Depends on cache size it used for reassemble
 *  IPv6 packets simultaneously.
 */
struct frag_cache {
	struct k_delayed_work timer;	/* Reassemble timer */
	struct net_pkt *pkt;		/* Reassemble packet */
	u16_t size;			/* Datagram size */
	u16_t tag;			/* Datagram tag */
	bool used;
};

static struct frag_cache cache[REASS_CACHE_SIZE];

/**
 *  RFC 4944, section 5.3
 *  If an entire payload (e.g., IPv6) datagram fits within a single 802.15.4
 *  frame, it is unfragmented and the LoWPAN encapsulation should not contain
 *  a fragmentation header.  If the datagram does not fit within a single
 *  IEEE 802.15.4 frame, it SHALL be broken into link fragments.  As the
 *  fragment offset can only express multiples of eight bytes, all link
 *  fragments for a datagram except the last one MUST be multiples of eight
 *  bytes in length.
 *
 *  RFC 7668, section 3 (IPv6 over Bluetooth Low Energy)
 *  Functionality is comprised of link-local IPv6 addresses and stateless
 *  IPv6 address autoconfiguration, Neighbor Discovery, and header compression
 *  Fragmentation features from 6LoWPAN standards are not used due to Bluetooth
 *  LE's link-layer fragmentation support.
 */

/**
 *                     1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |1 1 0 0 0|    datagram_size    |         datagram_tag          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *                     1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |1 1 0 0 0|    datagram_size    |         datagram_tag          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |datagram_offset|
 *  +-+-+-+-+-+-+-+-+
 */

static inline void set_datagram_size(u8_t *ptr, u16_t size)
{
	ptr[0] |= ((size & 0x7FF) >> 8);
	ptr[1] = (u8_t) size;
}

static inline void set_datagram_tag(u8_t *ptr, u16_t tag)
{
	ptr[0] = tag >> 8;
	ptr[1] = (u8_t) tag;
}

static inline void set_up_frag_hdr(struct net_buf *frag, u16_t size,
				   u8_t offset)
{
	u8_t pos = frag->len;

	if (!offset) {
		net_buf_add(frag, NET_6LO_FRAG1_HDR_LEN);
		frag->data[pos] = NET_6LO_DISPATCH_FRAG1;
	} else {
		net_buf_add(frag, NET_6LO_FRAGN_HDR_LEN);
		frag->data[pos] = NET_6LO_DISPATCH_FRAGN;
	}

	set_datagram_size(frag->data + pos, size);
	pos += NET_6LO_FRAG_DATAGRAM_SIZE_LEN;

	set_datagram_tag(frag->data + pos, datagram_tag);
	pos += NET_6LO_FRAG_DATAGRAM_OFFSET_LEN;

	if (offset) {
		frag->data[pos] = offset;
	}
}

static inline u8_t calc_max_payload(struct net_buf *frag, u8_t offset)
{
	u8_t max = frag->size - frag->len;

	return (max & 0xF8);
}

static inline u8_t copy_data(struct ieee802154_fragment_ctx *ctx,
			     struct net_buf *frame_buf, u8_t max)
{
	u8_t move = ctx->frag->len - (ctx->pos - ctx->frag->data);

	move = min(move, max);

	memcpy(frame_buf->data + frame_buf->len, ctx->pos, move);

	net_buf_add(frame_buf, move);

	return move;
}

static inline void update_fragment_ctx(struct ieee802154_fragment_ctx *ctx,
				       u8_t move)
{
	if (move == (ctx->frag->len - (ctx->pos - ctx->frag->data))) {
		ctx->frag = ctx->frag->frags;
		if (ctx->frag) {
			ctx->pos = ctx->frag->data;
		}
	} else {
		ctx->pos += move;
	}
}

/**
 *  ch  : compressed (IPv6) header(s)
 *  fh  : fragment header (dispatch + size + tag + [offset])
 *  p   : payload (first fragment holds IPv6 hdr as payload)
 *  e   : empty space
 *  ll  : link layer
 *
 *  Input frame_buf to ieee802154_fragment() looks like below
 *
 *  | ll |
 *
 *  After fragment creation, frame_buf will look like:
 *
 *  | ll + fh + p + e |
 *
 *  p being taken from current pkt buffer and position.
 *
 *  Space in every fragment is because fragment payload should be multiple
 *  of 8 octets (we have predefined packets at compile time, data packet mtu
 *  is set already).
 *
 *  If it's the first fragment being created, fh will not own any offset
 *  (so it will be 1 byte smaller)
 */
void ieee802154_fragment(struct ieee802154_fragment_ctx *ctx,
			 struct net_buf *frame_buf, bool iphc)
{
	u8_t max;

	if (!ctx->offset) {
		datagram_tag++;
	}

	set_up_frag_hdr(frame_buf, ctx->pkt_size, ctx->offset);
	max = calc_max_payload(frame_buf, ctx->offset);

	ctx->processed += max;

	if (!ctx->offset) {
		/* First fragment needs to take into account 6lo */
		if (iphc) {
			max -= ctx->hdr_diff;
		} else {
			/* Adding IPv6 dispatch header */
			max += 1;
		}
	}

	while (max && ctx->frag) {
		u8_t move;

		move = copy_data(ctx, frame_buf, max);

		update_fragment_ctx(ctx, move);

		max -= move;
	}

	ctx->offset = ctx->processed >> 3;
}

static inline u16_t get_datagram_size(u8_t *ptr)
{
	return ((ptr[0] & 0x1F) << 8) | ptr[1];
}

static inline u16_t get_datagram_tag(u8_t *ptr)
{
	return (ptr[0] << 8) | ptr[1];
}

static inline void remove_frag_header(struct net_buf *frag, u8_t hdr_len)
{
	memmove(frag->data, frag->data + hdr_len, frag->len - hdr_len);
	frag->len -= hdr_len;
}

static void update_protocol_header_lengths(struct net_pkt *pkt, u16_t size)
{
	net_pkt_set_ip_hdr_len(pkt, NET_IPV6H_LEN);

	NET_IPV6_HDR(pkt)->len = htons(size - NET_IPV6H_LEN);

	if (NET_IPV6_HDR(pkt)->nexthdr == IPPROTO_UDP) {
		struct net_udp_hdr hdr, *udp_hdr;

		udp_hdr = net_udp_get_hdr(pkt, &hdr);
		if (udp_hdr) {
			udp_hdr->len = htons(size - NET_IPV6H_LEN);

			net_udp_set_hdr(pkt, udp_hdr);
		} else {
			NET_ERR("could not get UDP header");
		}
	}
}

static inline void clear_reass_cache(u16_t size, u16_t tag)
{
	u8_t i;

	for (i = 0U; i < REASS_CACHE_SIZE; i++) {
		if (!(cache[i].size == size && cache[i].tag == tag)) {
			continue;
		}

		if (cache[i].pkt) {
			net_pkt_unref(cache[i].pkt);
		}

		cache[i].pkt = NULL;
		cache[i].size = 0U;
		cache[i].tag = 0U;
		cache[i].used = false;
		k_delayed_work_cancel(&cache[i].timer);
	}
}

/**
 *  If the reassembly not completed within reassembly timeout discard
 *  the whole packet.
 */
static void reass_timeout(struct k_work *work)
{
	struct frag_cache *cache = CONTAINER_OF(work, struct frag_cache, timer);

	if (cache->pkt) {
		net_pkt_unref(cache->pkt);
	}

	cache->pkt = NULL;
	cache->size = 0U;
	cache->tag = 0U;
	cache->used = false;
}

/**
 *  Upon reception of first fragment with respective of size and tag
 *  create a new cache. If number of unused cache are out then
 *  discard the fragments.
 */
static inline struct frag_cache *set_reass_cache(struct net_pkt *pkt,
						 u16_t size, u16_t tag)
{
	int i;

	for (i = 0; i < REASS_CACHE_SIZE; i++) {
		if (cache[i].used) {
			continue;
		}

		cache[i].pkt = pkt;
		cache[i].size = size;
		cache[i].tag = tag;
		cache[i].used = true;

		k_delayed_work_init(&cache[i].timer, reass_timeout);
		k_delayed_work_submit(&cache[i].timer, FRAG_REASSEMBLY_TIMEOUT);
		return &cache[i];
	}

	return NULL;
}

/**
 *  Return cache if it matches with size and tag of stored caches,
 *  otherwise return NULL.
 */
static inline struct frag_cache *get_reass_cache(u16_t size, u16_t tag)
{
	u8_t i;

	for (i = 0U; i < REASS_CACHE_SIZE; i++) {
		if (cache[i].used) {
			if (cache[i].size == size &&
			    cache[i].tag == tag) {
				return &cache[i];
			}
		}
	}

	return NULL;
}

/* Helper function to write fragment data to Rx packet based on offset. */
static inline bool copy_frag(struct net_pkt *pkt,
			     struct net_buf *frag,
			     u16_t offset)
{
	struct net_buf *input = frag;
	struct net_buf *write;
	u16_t pos = offset;

	write = pkt->frags;

	while (input) {
		write = net_pkt_write(pkt, write, pos, &pos, input->len,
				       input->data, NET_6LO_RX_PKT_TIMEOUT);
		if (!write && pos == 0xffff) {
			/* Free the new bufs we tried to get, we need to discard
			 * the whole fragment chain.
			 */
			net_pkt_frag_unref(pkt->frags);
			pkt->frags = NULL;

			return false;
		}

		input = input->frags;
	}

	net_pkt_frag_unref(frag);

	return true;
}

/**
 *  Parse size and tag from the fragment, check if we have any cache
 *  related to it. If not create a new cache.
 *  Remove the fragmentation header and uncompress IPv6 and related headers.
 *  Cache Rx part of fragment along with data buf for the first fragment
 *  in the cache, remaining fragments just cache data fragment, unref
 *  RX pkt. So in both the cases caller can assume packet is consumed.
 */
static inline enum net_verdict add_frag_to_cache(struct net_pkt *pkt,
						 bool first)
{
	struct frag_cache *cache;
	struct net_buf *frag;
	u16_t size;
	u16_t tag;
	u16_t offset = 0U;
	u8_t pos = 0U;

	/* Parse total size of packet */
	size = get_datagram_size(pkt->frags->data);
	pos += NET_6LO_FRAG_DATAGRAM_SIZE_LEN;

	/* Parse the datagram tag */
	tag = get_datagram_tag(pkt->frags->data + pos);
	pos += NET_6LO_FRAG_DATAGRAM_OFFSET_LEN;

	if (!first) {
		offset = ((u16_t)pkt->frags->data[pos]) << 3;
		pos++;
	}

	/* Remove frag header and update data */
	remove_frag_header(pkt->frags, pos);

	/* Uncompress the IP headers */
	if (first && !net_6lo_uncompress(pkt)) {
		NET_ERR("Could not uncompress first frag's 6lo hdr");
		clear_reass_cache(size, tag);

		return NET_DROP;
	}

	/* If there are no fragments in the cache means this frag
	 * is the first one. So cache Rx pkt otherwise not.
	 * Write data fragment data to cached Rx based on offset parameter.
	 * (Detach data fragment from incoming Rx and copy that data).
	 */

	frag = pkt->frags;
	pkt->frags = NULL;

	cache = get_reass_cache(size, tag);
	if (!cache) {
		cache = set_reass_cache(pkt, size, tag);
		if (!cache) {
			NET_ERR("Could not get a cache entry");
			pkt->frags = frag;
			return NET_DROP;
		}

		/* If write failed, then attach frag back to incoming packet
		 * and return NET_DROP, caller will take care of freeing it.
		 */
		if (!copy_frag(cache->pkt, frag, offset)) {
			pkt->frags = frag;

			/* Initialize to NULL to prevent double free. It's only
			 * needed here because this is the first fragment.
			 */
			cache->pkt = NULL;

			clear_reass_cache(size, tag);

			NET_ERR("Copying frag failed");

			return NET_DROP;
		}

		NET_DBG("packet inserted into cache");

		return NET_OK;
	}

	/* Add data packet to reassembly packet */
	if (!copy_frag(cache->pkt, frag, offset)) {
		pkt->frags = frag;

		clear_reass_cache(size, tag);

		return NET_DROP;
	}

	/* Check if all the fragments are received or not */
	if (net_pkt_get_len(cache->pkt) == size) {
		/* Assign frags back to input packet. */
		pkt->frags = cache->pkt->frags;
		cache->pkt->frags = NULL;

		/* Lengths are elided in compression, so calculate it. */
		update_protocol_header_lengths(pkt, cache->size);

		/* Once reassemble is done, cache is no longer needed. */
		clear_reass_cache(size, tag);

		NET_DBG("All fragments received and reassembled");

		return NET_CONTINUE;
	}

	/* Unref Rx part of original packet */
	net_pkt_unref(pkt);

	return NET_OK;
}

enum net_verdict ieee802154_reassemble(struct net_pkt *pkt)
{
	if (!pkt || !pkt->frags) {
		NET_ERR("Nothing to reassemble");
		return NET_DROP;
	}

	switch (pkt->frags->data[0] & 0xF0) {
	case NET_6LO_DISPATCH_FRAG1:
		/* First fragment with IP headers */
		return add_frag_to_cache(pkt, true);
	case NET_6LO_DISPATCH_FRAGN:
		/* Further fragments */
		return add_frag_to_cache(pkt, false);
	default:
		NET_DBG("No frag dispatch (%02x)", pkt->frags->data[0]);
		/* Received unfragmented packet, uncompress */
		if (net_6lo_uncompress(pkt)) {
			return NET_CONTINUE;
		}

		NET_ERR("Could not uncompress. Bogus packet?");
	}

	return NET_DROP;
}
