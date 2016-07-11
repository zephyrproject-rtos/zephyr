/** @file
 * @brief 802.15.4 fragment related functions
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

#if defined(CONFIG_NET_L2_IEEE802154_FRAGMENT_DEBUG)
#define SYS_LOG_DOMAIN "net/fragment"
#define NET_DEBUG 1
#endif

#include <errno.h>
#include <net/net_core.h>
#include <net/nbuf.h>
#include <net/net_if.h>
#include <net/net_stats.h>

#include "net_private.h"
#include "fragment.h"
#include "6lo.h"
#include "6lo_private.h"

static uint16_t datagram_tag;

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

static inline uint8_t calc_max_payload(struct net_buf *buf, uint8_t offset)
{
	uint8_t max;

	max = net_nbuf_iface(buf)->mtu - net_nbuf_ll_reserve(buf);
	max -= offset ? NET6LO_FRAGN_HDR_LEN : NET6LO_FRAG1_HDR_LEN;

	return (max & 0xF8);
}

static inline void set_up_frag_hdr(struct net_buf *buf, uint16_t size,
				   uint8_t offset)
{
	uint8_t pos = 0;

	if (offset) {
		buf->data[pos] = NET6LO_DISPATCH_FRAGN;
	} else {
		buf->data[pos] = NET6LO_DISPATCH_FRAG1;
	}

	set_datagram_size(buf->data, size);
	pos += NET6LO_FRAG_DATAGRAM_SIZE_LEN;

	set_datagram_tag(buf->data + pos, datagram_tag);
	pos += NET6LO_FRAG_DATAGRAM_OFFSET_LEN;

	if (offset) {
		buf->data[pos] = offset;
	}
}

static inline uint8_t compact_frag(struct net_buf *frag, uint8_t moved)
{
	uint8_t remaining;

	remaining = frag->len - moved;

	/** Move remaining data to next to fragmentation header,
	 *  (leaving space for header).
	 */
	memmove(frag->data + NET6LO_FRAGN_HDR_LEN, frag->data + moved,
		remaining);
	frag->len = NET6LO_FRAGN_HDR_LEN + remaining;

	return remaining;
}

static inline uint8_t move_frag_data(struct net_buf *frag,
				     struct net_buf *next,
				     uint8_t max, uint8_t data_len)
{
	uint8_t move, remaining;

	/* Calculate remaining space for data to move */
	remaining = max - data_len;
	move = next->len > remaining ? remaining : next->len;

	memmove(frag->data + frag->len, next->data, move);
	net_buf_add(frag, move);

	return move;
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
	uint16_t size;
	uint16_t offset;
	uint8_t move;
	uint8_t pos;
	uint8_t max;

	datagram_tag++;

	if (!buf || !buf->frags) {
		return false;
	}

	/* If it is a single fragment do not add fragmentation header */
	if (!buf->frags->frags) {
		return true;
	}

	/* Datagram_size: total length before compression */
	size = net_buf_frags_len(buf) + hdr_diff;

	frag = net_nbuf_get_reserve_data(net_nbuf_ll_reserve(buf));
	if (!frag) {
		return false;
	}

	/* Reserve space for fragmentation header */
	net_buf_add(frag, NET6LO_FRAG1_HDR_LEN);
	net_buf_frag_insert(buf, frag);

	offset = pos = 0;
	next = frag->frags;

	while (1) {
		/* Set fragmentation header in the beginning */
		set_up_frag_hdr(frag, size, offset >> 3);

		/* Calculate max payload in multiples of 8 bytes */
		max = calc_max_payload(buf, offset >> 3);

		/* Move data from next fragment to current fragment */
		move = move_frag_data(frag, next, max, pos);

		/* Compact the next fragment, returns the position of
		 * data offset */
		pos = compact_frag(next, move);

		/* Keep track of offset how much data is processed */
		offset += max + hdr_diff;

		/** If next fragment is last and data already moved to previous
		 *  fragment, then delete this fragment, if data is left insert
		 *  header.
		 */
		if (!next->frags) {
			if (next->len == NET6LO_FRAGN_HDR_LEN) {
				net_buf_frag_del(frag, next);
			} else {
				set_up_frag_hdr(next, size, offset >> 3);
			}

			break;
		}

		/* Repeat the steps */
		frag = next;
		next = next->frags;
	}

	return true;
}
