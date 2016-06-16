/*
 * Copyright (c) 2016 Intel Corporation.
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

/**
 * @file
 * @brief IEEE 802.15.4 Frame API header
 */

#ifndef __IEEE802154_FRAME_H__
#define __IEEE802154_FRAME_H__

#include <nanokernel.h>
#include <net/nbuf.h>

#define IEEE802154_MTU				127
#define IEEE802154_MIN_LENGTH			5
/* See Section 5.2.1.4 */
#define IEEE802154_BROADCAST_ADDRESS		0xFFFF
/* ACK packet size is the minimum size, see Section 5.2.2.3 */
#define IEEE802154_ACK_PKT_LENGTH		IEEE802154_MIN_LENGTH
#define IEEE802154_MFR_LENGTH			2

#define IEEE802154_EXT_ADDR_LENGTH		8
#define IEEE802154_SHORT_ADDR_LENGTH		2
#define IEEE802154_PAN_ID_LENGTH		2

/* See Section 5.2.1.1.1 */
enum ieee802154_frame_type {
	IEEE802154_FRAME_TYPE_BEACON		= 0x0,
	IEEE802154_FRAME_TYPE_DATA		= 0x1,
	IEEE802154_FRAME_TYPE_ACK		= 0x2,
	IEEE802154_FRAME_TYPE_MAC_COMMAND	= 0x3,
	IEEE802154_FRAME_TYPE_RESERVED		= 0x4,
};

/* See Section 5.2.1.1.6 */
enum ieee802154_addressing_mode {
	IEEE802154_ADDR_MODE_NONE		= 0x0,
	IEEE802154_ADDR_MODE_RESERVED		= 0x1,
	IEEE802154_ADDR_MODE_SHORT		= 0x2,
	IEEE802154_ADDR_MODE_EXTENDED		= 0x3,
};

/* See Section 5.2.1.1.7 */
enum ieee802154_version {
	IEEE802154_VERSION_802154_2003		= 0x0,
	IEEE802154_VERSION_802154		= 0x1,
	IEEE802154_VERSION_RESERVED		= 0x2,
};

/*
 * Frame Control Field and sequence number
 * See Section 5.2.1.1
 */
struct ieee802154_fcf_seq {
	struct {
		uint16_t frame_type		:3;
		uint16_t security_enabled	:1;
		uint16_t frame_pending		:1;
		uint16_t ar			:1;
		uint16_t pan_id_comp		:1;
		uint16_t reserved		:3;
		uint16_t dst_addr_mode		:2;
		uint16_t frame_version		:2;
		uint16_t src_addr_mode		:2;
	} fc __packed;

	uint8_t sequence;
} __packed;

struct ieee802154_address {
	union {
		uint16_t short_addr;
		uint8_t ext_addr[0];
	};
} __packed;

struct ieee802154_address_field_comp {
	struct ieee802154_address addr;
} __packed;

struct ieee802154_address_field_plain {
	uint16_t pan_id;
	struct ieee802154_address addr;
} __packed;

struct ieee802154_address_field {
	union {
		struct ieee802154_address_field_plain plain;
		struct ieee802154_address_field_comp comp;
	};
} __packed;

/** MAC header */
struct ieee802154_mhr {
	struct ieee802154_fcf_seq *fs;
	struct ieee802154_address_field *dst_addr;
	struct ieee802154_address_field *src_addr;
};

struct ieee802154_mfr {
	uint16_t fcs;
};

/** Frame */
struct ieee802154_mpdu {
	struct ieee802154_mhr mhr;
	void *payload;
	struct ieee802154_mfr *mfr;
} __packed;

bool ieee802154_validate_frame(uint8_t *buf, uint8_t length,
			       struct ieee802154_mpdu *mpdu);

uint16_t ieee802154_compute_header_size(struct net_if *iface,
					struct in6_addr *dst);

bool ieee802154_create_data_frame(struct net_if *iface,
				  struct net_buf *buf);

static inline bool ieee802154_ack_required(struct net_buf *buf)
{
	struct ieee802154_fcf_seq *fs =
		(struct ieee802154_fcf_seq *)net_nbuf_ll(buf);

	return fs->fc.ar;
}

#endif /* __IEEE802154_FRAME_H__ */
