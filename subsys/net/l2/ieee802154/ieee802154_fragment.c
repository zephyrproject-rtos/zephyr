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

#define NET_FRAG_DISPATCH_MASK	0xF8
#define NET_FRAG_OFFSET_POS	(NET_6LO_FRAG_DATAGRAM_SIZE_LEN +	\
				 NET_6LO_FRAG_DATAGRAM_OFFSET_LEN)

#define BUF_TIMEOUT K_MSEC(50)

#define FRAG_REASSEMBLY_TIMEOUT \
	K_SECONDS(CONFIG_NET_L2_IEEE802154_REASSEMBLY_TIMEOUT)
#define REASS_CACHE_SIZE CONFIG_NET_L2_IEEE802154_FRAGMENT_REASS_CACHE_SIZE

static uint16_t datagram_tag;

/**
 *  Reassemble cache : Depends on cache size it used for reassemble
 *  IPv6 packets simultaneously.
 */
struct frag_cache {
	struct k_delayed_work timer;	/* Reassemble timer */
	struct net_pkt *pkt;		/* Reassemble packet */
	uint16_t size;			/* Datagram size */
	uint16_t tag;			/* Datagram tag */
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

static inline void set_datagram_size(uint8_t *ptr, uint16_t size)
{
	ptr[0] |= ((size & 0x7FF) >> 8);
	ptr[1] = (uint8_t) size;
}

static inline void set_datagram_tag(uint8_t *ptr, uint16_t tag)
{
	ptr[0] = tag >> 8;
	ptr[1] = (uint8_t) tag;
}

static inline void set_up_frag_hdr(struct net_buf *frag, uint16_t size,
				   uint8_t offset)
{
	uint8_t pos = frag->len;

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

static inline uint8_t calc_max_payload(struct net_buf *frag, uint8_t offset)
{
	uint8_t max = frag->size - frag->len;

	return (max & 0xF8);
}

static inline uint8_t copy_data(struct ieee802154_fragment_ctx *ctx,
			     struct net_buf *frame_buf, uint8_t max)
{
	uint8_t move = ctx->buf->len - (ctx->pos - ctx->buf->data);

	move = MIN(move, max);

	memcpy(frame_buf->data + frame_buf->len, ctx->pos, move);

	net_buf_add(frame_buf, move);

	return move;
}

static inline void update_fragment_ctx(struct ieee802154_fragment_ctx *ctx,
				       uint8_t move)
{
	if (move == (ctx->buf->len - (ctx->pos - ctx->buf->data))) {
		ctx->buf = ctx->buf->frags;
		if (ctx->buf) {
			ctx->pos = ctx->buf->data;
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
	uint8_t max;

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
			max += 1U;
		}
	}

	while (max && ctx->buf) {
		uint8_t move;

		move = copy_data(ctx, frame_buf, max);

		update_fragment_ctx(ctx, move);

		max -= move;
	}

	ctx->offset = ctx->processed >> 3;
}

static inline uint16_t get_datagram_size(uint8_t *ptr)
{
	return ((ptr[0] & 0x1F) << 8) | ptr[1];
}

static inline uint16_t get_datagram_tag(uint8_t *ptr)
{
	return (ptr[0] << 8) | ptr[1];
}

static void update_protocol_header_lengths(struct net_pkt *pkt, uint16_t size)
{
	NET_PKT_DATA_ACCESS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	struct net_ipv6_hdr *ipv6;

	ipv6 = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &ipv6_access);
	if (!ipv6) {
		NET_ERR("could not get IPv6 header");
		return;
	}

	net_pkt_set_ip_hdr_len(pkt, NET_IPV6H_LEN);
	ipv6->len = htons(size - NET_IPV6H_LEN);

	net_pkt_set_data(pkt, &ipv6_access);

	if (ipv6->nexthdr == IPPROTO_UDP) {
		NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
		struct net_udp_hdr *udp;

		udp = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_access);
		if (udp) {
			udp->len = htons(size - NET_IPV6H_LEN);
			net_pkt_set_data(pkt, &udp_access);
		} else {
			NET_ERR("could not get UDP header");
		}
	}
}

static inline void clear_reass_cache(uint16_t size, uint16_t tag)
{
	uint8_t i;

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
						 uint16_t size, uint16_t tag)
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
static inline struct frag_cache *get_reass_cache(uint16_t size, uint16_t tag)
{
	uint8_t i;

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

static inline void fragment_append(struct net_pkt *pkt, struct net_buf *frag)
{
	if ((frag->data[0] & NET_FRAG_DISPATCH_MASK) ==
	    NET_6LO_DISPATCH_FRAG1) {
		/* Always make sure first fragment is inserted first
		 * This will be useful for fragment_cached_pkt_len()
		 */
		frag->frags = pkt->buffer;
		pkt->buffer = frag;
	} else {
		net_pkt_append_buffer(pkt, frag);
	}
}

static inline size_t fragment_cached_pkt_len(struct net_pkt *pkt)
{
	size_t len = 0U;
	struct net_buf *frag;
	int hdr_len;
	uint8_t *data;

	frag = pkt->buffer;
	while (frag) {
		uint16_t hdr_len = NET_6LO_FRAGN_HDR_LEN;

		if ((frag->data[0] & NET_FRAG_DISPATCH_MASK) ==
		    NET_6LO_DISPATCH_FRAG1) {
			hdr_len = NET_6LO_FRAG1_HDR_LEN;
		}

		len += frag->len - hdr_len;

		frag = frag->frags;
	}

	/* 6lo assumes that fragment header has been removed,
	 * and in our side we assume first buffer is always the first fragment.
	 */
	data = pkt->buffer->data;
	pkt->buffer->data += NET_6LO_FRAG1_HDR_LEN;

	hdr_len = net_6lo_uncompress_hdr_diff(pkt);

	pkt->buffer->data = data;

	if (hdr_len == INT_MAX) {
		return 0;
	}

	return len + hdr_len;
}

static inline uint16_t fragment_offset(struct net_buf *frag)
{
	if ((frag->data[0] & NET_FRAG_DISPATCH_MASK) ==
		    NET_6LO_DISPATCH_FRAG1) {
		return 0;
	}

	return ((uint16_t)frag->data[NET_FRAG_OFFSET_POS] << 3);
}

static void fragment_move_back(struct net_pkt *pkt,
			       struct net_buf *frag, struct net_buf *stop)
{
	struct net_buf *prev, *current;

	prev = NULL;
	current = pkt->buffer;

	while (current && current != stop) {
		if (fragment_offset(frag) < fragment_offset(current)) {
			if (prev) {
				prev->frags = frag;
			}

			frag->frags = current;
			break;
		}

		prev = current;
		current = current->frags;
	}
}

static inline void fragment_remove_headers(struct net_pkt *pkt)
{
	struct net_buf *frag;

	frag = pkt->buffer;
	while (frag) {
		uint16_t hdr_len = NET_6LO_FRAGN_HDR_LEN;

		if ((frag->data[0] & NET_FRAG_DISPATCH_MASK) ==
		    NET_6LO_DISPATCH_FRAG1) {
			hdr_len = NET_6LO_FRAG1_HDR_LEN;
		}

		memmove(frag->data, frag->data + hdr_len, frag->len - hdr_len);
		frag->len -= hdr_len;

		frag = frag->frags;
	}
}

static inline void fragment_reconstruct_packet(struct net_pkt *pkt)
{
	struct net_buf *prev, *current, *next;

	prev = NULL;
	current = pkt->buffer;

	while (current) {
		next = current->frags;

		if (!prev || (fragment_offset(prev) >
			      fragment_offset(current))) {
			prev = current;
		} else {
			fragment_move_back(pkt, current, prev);
		}

		current = next;
	}

	/* Let's remove now useless fragmentation headers */
	fragment_remove_headers(pkt);
}

/**
 *  Parse size and tag from the fragment, check if we have any cache
 *  related to it. If not create a new cache.
 *  Remove the fragmentation header and uncompress IPv6 and related headers.
 *  Cache Rx part of fragment along with data buf for the first fragment
 *  in the cache, remaining fragments just cache data fragment, unref
 *  RX pkt. So in both the cases caller can assume packet is consumed.
 */
static inline enum net_verdict fragment_add_to_cache(struct net_pkt *pkt)
{
	bool first_frag = false;
	struct frag_cache *cache;
	struct net_buf *frag;
	uint16_t size;
	uint16_t tag;

	/* Parse total size of packet */
	size = get_datagram_size(pkt->buffer->data);

	/* Parse the datagram tag */
	tag = get_datagram_tag(pkt->buffer->data +
			       NET_6LO_FRAG_DATAGRAM_SIZE_LEN);

	/* If there are no fragments in the cache means this frag
	 * is the first one. So cache Rx pkt otherwise not.
	 */
	frag = pkt->buffer;
	pkt->buffer = NULL;

	cache = get_reass_cache(size, tag);
	if (!cache) {
		cache = set_reass_cache(pkt, size, tag);
		if (!cache) {
			NET_ERR("Could not get a cache entry");
			pkt->buffer = frag;
			return NET_DROP;
		}

		first_frag = true;
	}

	fragment_append(cache->pkt, frag);

	if (fragment_cached_pkt_len(cache->pkt) == cache->size) {
		/* Assign buffer back to input packet. */
		pkt->buffer = cache->pkt->buffer;
		cache->pkt->buffer = NULL;

		fragment_reconstruct_packet(pkt);

		/* Once reassemble is done, cache is no longer needed. */
		clear_reass_cache(size, tag);

		if (!net_6lo_uncompress(pkt)) {
			NET_ERR("Could not uncompress. Bogus packet?");
			return NET_DROP;
		}

		net_pkt_cursor_init(pkt);

		update_protocol_header_lengths(pkt, size);

		net_pkt_cursor_init(pkt);

		NET_DBG("All fragments received and reassembled");

		return NET_CONTINUE;
	}

	/* Unref Rx part of original packet */
	if (!first_frag) {
		net_pkt_unref(pkt);
	}

	return NET_OK;
}


enum net_verdict ieee802154_reassemble(struct net_pkt *pkt)
{
	if (!pkt || !pkt->buffer) {
		NET_ERR("Nothing to reassemble");
		return NET_DROP;
	}

	if ((pkt->buffer->data[0] & NET_FRAG_DISPATCH_MASK) >=
	    NET_6LO_DISPATCH_FRAG1) {
		return fragment_add_to_cache(pkt);
	} else {
		NET_DBG("No frag dispatch (%02x)", pkt->buffer->data[0]);
		/* Received unfragmented packet, uncompress */
		if (net_6lo_uncompress(pkt)) {
			return NET_CONTINUE;
		}

		NET_ERR("Could not uncompress. Bogus packet?");
	}

	return NET_DROP;
}
