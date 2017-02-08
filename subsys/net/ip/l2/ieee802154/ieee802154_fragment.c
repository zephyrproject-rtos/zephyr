/** @file
 * @brief 802.15.4 fragment related functions
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_L2_IEEE802154_FRAGMENT)
#define SYS_LOG_DOMAIN "net/ieee802154"
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>
#include <net/net_core.h>
#include <net/nbuf.h>
#include <net/net_if.h>
#include <net/net_stats.h>

#include "ieee802154_fragment.h"

#include "net_private.h"
#include "6lo.h"
#include "6lo_private.h"

#define FRAG_REASSEMBLY_TIMEOUT (MSEC_PER_SEC * \
				 CONFIG_NET_L2_IEEE802154_REASSEMBLY_TIMEOUT)
#define REASS_CACHE_SIZE CONFIG_NET_L2_IEEE802154_FRAGMENT_REASS_CACHE_SIZE

static uint16_t datagram_tag;

/**
 *  Reasseble cache : Depends on cache size it used for reassemble
 *  IPv6 packets simultaneously.
 */
struct frag_cache {
	struct k_delayed_work timer;	/* Reassemble timer */
	struct net_buf *buf;		/* Reassemble buffer */
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

static inline struct net_buf *prepare_new_fragment(struct net_buf *buf,
						   uint8_t offset)
{
	struct net_buf *frag;

	frag = net_nbuf_get_reserve_data(net_nbuf_ll_reserve(buf), K_FOREVER);
	if (!frag) {
		return NULL;
	}

	/* Reserve space for fragmentation header */
	if (!offset) {
		net_buf_add(frag, NET_6LO_FRAG1_HDR_LEN);
	} else {
		net_buf_add(frag, NET_6LO_FRAGN_HDR_LEN);
	}

	net_buf_frag_add(buf, frag);

	return frag;
}

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
	uint8_t pos = 0;

	if (offset) {
		frag->data[pos] = NET_6LO_DISPATCH_FRAGN;
	} else {
		frag->data[pos] = NET_6LO_DISPATCH_FRAG1;
	}

	set_datagram_size(frag->data, size);
	pos += NET_6LO_FRAG_DATAGRAM_SIZE_LEN;

	set_datagram_tag(frag->data + pos, datagram_tag);
	pos += NET_6LO_FRAG_DATAGRAM_OFFSET_LEN;

	if (offset) {
		frag->data[pos] = offset;
	}
}

static inline uint8_t calc_max_payload(struct net_buf *buf,
				       struct net_buf *frag,
				       uint8_t offset)
{
	uint8_t max;

	max = frag->size - net_nbuf_ll_reserve(buf);
	max -= offset ? NET_6LO_FRAGN_HDR_LEN : NET_6LO_FRAG1_HDR_LEN;

	return (max & 0xF8);
}

static inline uint8_t move_frag_data(struct net_buf *frag,
				     struct net_buf *next,
				     uint8_t max,
				     bool first,
				     int hdr_diff,
				     uint8_t *room_left)
{
	uint8_t room;
	uint8_t move;
	uint8_t occupied;

	/* First fragment */
	if (first) {
		occupied = frag->len - NET_6LO_FRAG1_HDR_LEN;
	} else {
		occupied = frag->len - NET_6LO_FRAGN_HDR_LEN;
	}

	/* Remaining room for data */
	room = max - occupied;

	if (first) {
		room -= hdr_diff;
	}

	/* Calculate remaining room space for data to move */
	move = next->len > room ? room : next->len;

	memmove(frag->data + frag->len, next->data, move);

	net_buf_add(frag, move);

	/* Room left in current fragment */
	*room_left = room - move;

	return move;
}

static inline void compact_frag(struct net_buf *frag, uint8_t moved)
{
	uint8_t remaining = frag->len - moved;

	/* Move remaining data next to fragmentation header,
	 * (leave space for header).
	 */
	if (remaining) {
		memmove(frag->data, frag->data + moved, remaining);
	}

	frag->len = remaining;
}

/**
 *  ch  : compressed (IPv6) header(s)
 *  fh  : fragment header (dispatch + size + tag + [offset])
 *  p   : payload (first fragment holds IPv6 hdr as payload)
 *  e   : empty space
 *
 *  Input to ieee802154_fragment() buf chain looks like below
 *
 *  | ch + p | p | p | p | p | p + e |
 *
 *  After complete fragmentation buf chain looks like below
 *
 *  |fh + p + e | fh + p + e | fh + p + e | fh + p + e | fh + p + e |
 *
 *  Space in every fragment is because fragment payload should be multiple
 *  of 8 octets (we have predefined buffers at compile time, data buffer mtu
 *  is set already).
 *
 *  Create the first fragment, add fragmentation header and insert
 *  fragment at beginning of buf, move data from next fragments to
 *  previous one, from here on insert fragmentation header and adjust
 *  data on subsequent buffers.
 */
bool ieee802154_fragment(struct net_buf *buf, int hdr_diff)
{
	struct net_buf *frag;
	struct net_buf *next;
	uint16_t processed;
	uint16_t offset;
	uint16_t size;
	uint8_t room;
	uint8_t move;
	uint8_t max;
	bool first;

	if (!buf || !buf->frags) {
		return false;
	}

	/* If it is a single fragment do not add fragmentation header */
	if (!buf->frags->frags) {
		return true;
	}

	/* Datagram_size: total length before compression */
	size = net_buf_frags_len(buf) + hdr_diff;

	room = 0;
	offset = 0;
	processed = 0;
	first = true;
	datagram_tag++;

	next = buf->frags;
	buf->frags = NULL;

	/* First fragment has compressed header, but SIZE and OFFSET
	 * values in fragmentation header are based on uncompressed
	 * IP packet.
	 */
	while (1) {
		if (!room) {
			/* Prepare new fragment based on offset */
			frag = prepare_new_fragment(buf, offset);
			if (!frag) {
				return false;
			}

			/* Set fragmentation header in the beginning */
			set_up_frag_hdr(frag, size, offset);

			/* Calculate max payload in multiples of 8 bytes */
			max = calc_max_payload(buf, frag, offset);

			/* Calculate how much data is processed */
			processed += max;

			offset = processed >> 3;
		}

		/* Move data from next fragment to current fragment */
		move = move_frag_data(frag, next, max, first, hdr_diff, &room);
		first = false;

		/* Compact the next fragment */
		compact_frag(next, move);

		if (!next->len) {
			next = net_buf_frag_del(NULL, next);
			if (!next) {
				break;
			}
		}
	}

	return true;
}

static inline uint16_t get_datagram_size(uint8_t *ptr)
{
	return ((ptr[0] & 0x1F) << 8) | ptr[1];
}

static inline uint16_t get_datagram_tag(uint8_t *ptr)
{
	return (ptr[0] << 8) | ptr[1];
}

static inline void remove_frag_header(struct net_buf *frag, uint8_t hdr_len)
{
	memmove(frag->data, frag->data + hdr_len, frag->len - hdr_len);
	frag->len -= hdr_len;
}

static void update_protocol_header_lengths(struct net_buf *buf, uint16_t size)
{
	net_nbuf_set_ip_hdr_len(buf, NET_IPV6H_LEN);

	NET_IPV6_BUF(buf)->len[0] = (size - NET_IPV6H_LEN) >> 8;
	NET_IPV6_BUF(buf)->len[1] = (uint8_t) (size - NET_IPV6H_LEN);

	if (NET_IPV6_BUF(buf)->nexthdr == IPPROTO_UDP) {
		NET_UDP_BUF(buf)->len = htons(size - NET_IPV6H_LEN);
	}
}

static inline void clear_reass_cache(uint16_t size, uint16_t tag)
{
	uint8_t i;

	for (i = 0; i < REASS_CACHE_SIZE; i++) {
		if (!(cache[i].size == size && cache[i].tag == tag)) {
			continue;
		}

		net_nbuf_unref(cache[i].buf);

		cache[i].buf = NULL;
		cache[i].size = 0;
		cache[i].tag = 0;
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

	if (cache->buf) {
		net_nbuf_unref(cache->buf);
	}

	cache->buf = NULL;
	cache->size = 0;
	cache->tag = 0;
	cache->used = false;
}

/**
 *  Upon receiption of first fragment with respective of size and tag
 *  create a new cache. If number of unused cache are out then
 *  discard the fragments.
 */
static inline struct frag_cache *set_reass_cache(struct net_buf *buf,
						 uint16_t size, uint16_t tag)
{
	int i;

	for (i = 0; i < REASS_CACHE_SIZE; i++) {
		if (cache[i].used) {
			continue;
		}

		cache[i].buf = buf;
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

	for (i = 0; i < REASS_CACHE_SIZE; i++) {
		if (cache[i].used) {
			if (cache[i].size == size &&
			    cache[i].tag == tag) {
				return &cache[i];
			}
		}
	}

	return NULL;
}

/* Helper function to write fragment data to Rx buffer based on offset. */
static inline bool copy_frag(struct net_buf *buf,
			     struct net_buf *frag,
			     uint16_t offset)
{
	struct net_buf *input = frag;
	struct net_buf *write;
	uint16_t pos = offset;

	write = buf->frags;

	while (input) {
		write = net_nbuf_write(buf, write, pos, &pos, input->len,
				       input->data, K_FOREVER);
		if (!write && pos == 0xffff) {
			return false;
		}

		input = input->frags;
	}

	net_nbuf_unref(frag);

	return true;
}

/**
 *  Parse size and tag from the fragment, check if we have any cache
 *  related to it. If not create a new cache.
 *  Remove the fragmentation header and uncompress IPv6 and related headers.
 *  Cache Rx part of fragment along with data buf for the first fragment
 *  in the cache, remaining fragments just cache data fragment, unref
 *  RX buf. So in both the cases caller can assume buffer is consumed.
 */
static inline enum net_verdict add_frag_to_cache(struct net_buf *buf,
						 bool first)
{
	struct frag_cache *cache;
	struct net_buf *frag;
	uint16_t size;
	uint16_t tag;
	uint16_t offset = 0;
	uint8_t pos = 0;

	/* Parse total size of packet */
	size = get_datagram_size(buf->frags->data);
	pos += NET_6LO_FRAG_DATAGRAM_SIZE_LEN;

	/* Parse the datagram tag */
	tag = get_datagram_tag(buf->frags->data + pos);
	pos += NET_6LO_FRAG_DATAGRAM_OFFSET_LEN;

	if (!first) {
		offset = ((uint16_t)buf->frags->data[pos]) << 3;
		pos++;
	}

	/* Remove frag header and update data */
	remove_frag_header(buf->frags, pos);

	/* Uncompress the IP headers */
	if (first && !net_6lo_uncompress(buf)) {
		NET_ERR("Could not uncompress first frag's 6lo hdr");
		clear_reass_cache(size, tag);

		return NET_DROP;
	}

	/* If there are no fragments in the cache means this frag
	 * is the first one. So cache Rx buf otherwise not.
	 * Write data fragment data to cached Rx based on offset parameter.
	 * (Detach data fragment from incoming Rx and copy that data).
	 */

	frag = buf->frags;
	buf->frags = NULL;

	cache = get_reass_cache(size, tag);
	if (!cache) {
		cache = set_reass_cache(buf, size, tag);
		if (!cache) {
			NET_ERR("Could not get a cache entry");
			return NET_DROP;
		}

		/* If write failed, then attach frag back to incoming buffer
		 * and return NET_DROP, caller will take care of freeing it.
		 */
		if (!copy_frag(cache->buf, frag, offset)) {
			buf->frags = frag;

			/* Initialize to NULL to prevent duble free. It's only
			 * needed here because this is the first fragment.
			 */
			cache->buf = NULL;

			clear_reass_cache(size, tag);

			NET_ERR("Copying frag failed");

			return NET_DROP;
		}

		NET_DBG("buffer inserted into cache");

		return NET_OK;
	}

	/* Add data buffer to reassembly buffer */
	if (!copy_frag(cache->buf, frag, offset)) {
		buf->frags = frag;

		clear_reass_cache(size, tag);

		return NET_DROP;
	}

	/* Check if all the fragments are received or not */
	if (net_buf_frags_len(cache->buf->frags) == size) {
		/* Assign frags back to input buffer. */
		buf->frags = cache->buf->frags;
		cache->buf->frags = NULL;

		/* Lengths are elided in compression, so calculate it. */
		update_protocol_header_lengths(buf, cache->size);

		/* Once reassemble is done, cache is no longer needed. */
		clear_reass_cache(size, tag);

		NET_DBG("All fragments received and reassembled");

		return NET_CONTINUE;
	}

	/* Unref Rx part of original buffer */
	net_nbuf_unref(buf);

	return NET_OK;
}

enum net_verdict ieee802154_reassemble(struct net_buf *buf)
{
	if (!buf || !buf->frags) {
		NET_ERR("Nothing to reassemble");
		return NET_DROP;
	}

	switch (buf->frags->data[0] & 0xF0) {
	case NET_6LO_DISPATCH_FRAG1:
		/* First fragment with IP headers */
		return add_frag_to_cache(buf, true);
	case NET_6LO_DISPATCH_FRAGN:
		/* Further fragments */
		return add_frag_to_cache(buf, false);
	default:
		NET_DBG("No frag dispatch (%02x)", buf->frags->data[0]);
		/* Received unfragmented packet, uncompress */
		if (net_6lo_uncompress(buf)) {
			return NET_CONTINUE;
		}

		NET_ERR("Could not uncompress. Bogus packet?");
	}

	return NET_DROP;
}
