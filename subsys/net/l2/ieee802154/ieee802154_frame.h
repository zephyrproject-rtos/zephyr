/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IEEE 802.15.4 MAC frame related functions
 *
 * This is not to be included by the application.
 */

#ifndef __IEEE802154_FRAME_H__
#define __IEEE802154_FRAME_H__

#include <zephyr/kernel.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/toolchain.h>

/* All specification references in this file refer to IEEE 802.15.4-2006
 * unless otherwise noted.
 */

#define IEEE802154_MIN_LENGTH	     3
/* ACK packet size is the minimum size, see section 7.2.2.3 */
#define IEEE802154_ACK_PKT_LENGTH    IEEE802154_MIN_LENGTH

#define IEEE802154_FCF_SEQ_LENGTH     3
#define IEEE802154_SIMPLE_ADDR_LENGTH 1
#define IEEE802154_PAN_ID_LENGTH      2

#define IEEE802154_BEACON_MIN_SIZE	  4
#define IEEE802154_BEACON_SF_SIZE	  2
#define IEEE802154_BEACON_GTS_SPEC_SIZE	  1
#define IEEE802154_BEACON_GTS_IF_MIN_SIZE IEEE802154_BEACON_GTS_SPEC_SIZE
#define IEEE802154_BEACON_PAS_SPEC_SIZE	  1
#define IEEE802154_BEACON_PAS_IF_MIN_SIZE IEEE802154_BEACON_PAS_SPEC_SIZE
#define IEEE802154_BEACON_GTS_DIR_SIZE	  1
#define IEEE802154_BEACON_GTS_SIZE	  3
#define IEEE802154_BEACON_GTS_RX	  1
#define IEEE802154_BEACON_GTS_TX	  0

/* See section 7.2.1.1.1 */
enum ieee802154_frame_type {
	IEEE802154_FRAME_TYPE_BEACON = 0x0,
	IEEE802154_FRAME_TYPE_DATA = 0x1,
	IEEE802154_FRAME_TYPE_ACK = 0x2,
	IEEE802154_FRAME_TYPE_MAC_COMMAND = 0x3,
	IEEE802154_FRAME_TYPE_LLDN = 0x4,
	IEEE802154_FRAME_TYPE_MULTIPURPOSE = 0x5,
	IEEE802154_FRAME_TYPE_RESERVED = 0x6,
};

/* See section 7.2.1.1.6 */
enum ieee802154_addressing_mode {
	IEEE802154_ADDR_MODE_NONE = 0x0,
	IEEE802154_ADDR_MODE_SIMPLE = 0x1,
	IEEE802154_ADDR_MODE_SHORT = 0x2,
	IEEE802154_ADDR_MODE_EXTENDED = 0x3,
};

/* Version 2006 (and before) do no support simple addressing mode */
#define IEEE802154_ADDR_MODE_RESERVED IEEE802154_ADDR_MODE_SIMPLE

/*
 * See IEEE 802.15.4-2006 section 7.2.1.1.7 and
 * IEEE 802.15.4-2015, section 7.2.1.9
 */
enum ieee802154_version {
	IEEE802154_VERSION_802154_2003 = 0x0,
	IEEE802154_VERSION_802154_2006 = 0x1,
	IEEE802154_VERSION_802154 = 0x2,
	IEEE802154_VERSION_RESERVED = 0x3,
};

/*
 * Frame Control Field and sequence number,
 * see section 7.2.1.1
 */
struct ieee802154_fcf_seq {
	struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		uint16_t frame_type : 3;
		uint16_t security_enabled : 1;
		uint16_t frame_pending : 1;
		uint16_t ar : 1;
		uint16_t pan_id_comp : 1;
		uint16_t reserved : 1;
		uint16_t seq_num_suppr : 1;
		uint16_t ie_list : 1;
		uint16_t dst_addr_mode : 2;
		uint16_t frame_version : 2;
		uint16_t src_addr_mode : 2;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		uint16_t reserved : 1;
		uint16_t pan_id_comp : 1;
		uint16_t ar : 1;
		uint16_t frame_pending : 1;
		uint16_t security_enabled : 1;
		uint16_t frame_type : 3;
		uint16_t src_addr_mode : 2;
		uint16_t frame_version : 2;
		uint16_t dst_addr_mode : 2;
		uint16_t ie_list : 1;
		uint16_t seq_num_suppr : 1;
#endif
	} fc __packed;

	uint8_t sequence;
} __packed;

struct ieee802154_address {
	union {
		uint8_t simple_addr;
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

/* See section 7.6.2.2.1 */
enum ieee802154_security_level {
	IEEE802154_SECURITY_LEVEL_NONE = 0x0,
	IEEE802154_SECURITY_LEVEL_MIC_32 = 0x1,
	IEEE802154_SECURITY_LEVEL_MIC_64 = 0x2,
	IEEE802154_SECURITY_LEVEL_MIC_128 = 0x3,
	IEEE802154_SECURITY_LEVEL_ENC = 0x4,
	IEEE802154_SECURITY_LEVEL_ENC_MIC_32 = 0x5,
	IEEE802154_SECURITY_LEVEL_ENC_MIC_64 = 0x6,
	IEEE802154_SECURITY_LEVEL_ENC_MIC_128 = 0x7,
};

/* This will match above *_MIC_<32/64/128> */
#define IEEE8021254_AUTH_TAG_LENGTH_32	4
#define IEEE8021254_AUTH_TAG_LENGTH_64	8
#define IEEE8021254_AUTH_TAG_LENGTH_128 16

/* See section 7.6.2.2.2 */
enum ieee802154_key_id_mode {
	IEEE802154_KEY_ID_MODE_IMPLICIT = 0x0,
	IEEE802154_KEY_ID_MODE_INDEX = 0x1,
	IEEE802154_KEY_ID_MODE_SRC_4_INDEX = 0x2,
	IEEE802154_KEY_ID_MODE_SRC_8_INDEX = 0x3,
};

#define IEEE8021254_KEY_ID_FIELD_INDEX_LENGTH	    1
#define IEEE8021254_KEY_ID_FIELD_SRC_4_INDEX_LENGTH 5
#define IEEE8021254_KEY_ID_FIELD_SRC_8_INDEX_LENGTH 9

#define IEEE802154_KEY_MAX_LEN 16

/* See section 7.6.2.2 */
struct ieee802154_security_control_field {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint8_t security_level : 3;
	uint8_t key_id_mode : 2;
	uint8_t reserved : 3;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint8_t reserved : 3;
	uint8_t key_id_mode : 2;
	uint8_t security_level : 3;
#endif
} __packed;

#define IEEE802154_SECURITY_CF_LENGTH 1

/* See section 7.6.2.4 */
struct ieee802154_key_identifier_field {
	union {
		/* mode_0 being implicit, it holds no info here */
		struct {
			uint8_t key_index;
		} mode_1;

		struct {
			uint8_t key_src[4];
			uint8_t key_index;
		} mode_2;

		struct {
			uint8_t key_src[8];
			uint8_t key_index;
		} mode_3;
	};
} __packed;

/*
 * Auxiliary Security Header
 * See section 7.6.2
 */
struct ieee802154_aux_security_hdr {
	struct ieee802154_security_control_field control;
	uint32_t frame_counter;
	struct ieee802154_key_identifier_field kif;
} __packed;

#define IEEE802154_SECURITY_FRAME_COUNTER_LENGTH 4

/* MAC header and footer, see section 7.2.1 */
struct ieee802154_mhr {
	struct ieee802154_fcf_seq *fs;
	struct ieee802154_address_field *dst_addr;
	struct ieee802154_address_field *src_addr;
#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	struct ieee802154_aux_security_hdr *aux_sec;
#endif
};

struct ieee802154_mfr {
	uint16_t fcs;
};

struct ieee802154_gts_dir {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint8_t mask : 7;
	uint8_t reserved : 1;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint8_t reserved : 1;
	uint8_t mask : 7;
#endif
} __packed;

struct ieee802154_gts {
	uint16_t short_address;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint8_t starting_slot : 4;
	uint8_t length : 4;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint8_t length : 4;
	uint8_t starting_slot : 4;
#endif
} __packed;

struct ieee802154_gts_spec {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	/* Descriptor Count */
	uint8_t desc_count : 3;
	uint8_t reserved : 4;
	/* GTS Permit */
	uint8_t permit : 1;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	/* GTS Permit */
	uint8_t permit : 1;
	uint8_t reserved : 4;
	/* Descriptor Count */
	uint8_t desc_count : 3;
#endif
} __packed;

struct ieee802154_pas_spec {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	/* Number of Short Addresses Pending */
	uint8_t nb_sap : 3;
	uint8_t reserved_1 : 1;
	/* Number of Extended Addresses Pending */
	uint8_t nb_eap : 3;
	uint8_t reserved_2 : 1;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint8_t reserved_1 : 1;
	/* Number of Extended Addresses Pending */
	uint8_t nb_eap : 3;
	uint8_t reserved_2 : 1;
	/* Number of Short Addresses Pending */
	uint8_t nb_sap : 3;
#endif
} __packed;

struct ieee802154_beacon_sf {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	/* Beacon Order*/
	uint16_t bc_order : 4;
	/* Superframe Order*/
	uint16_t sf_order : 4;
	/* Final CAP Slot */
	uint16_t cap_slot : 4;
	/* Battery Life Extension */
	uint16_t ble : 1;
	uint16_t reserved : 1;
	/* PAN Coordinator */
	uint16_t coordinator : 1;
	/* Association Permit */
	uint16_t association : 1;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	/* Superframe Order*/
	uint16_t sf_order : 4;
	/* Beacon Order*/
	uint16_t bc_order : 4;
	/* Association Permit */
	uint16_t association : 1;
	/* PAN Coordinator */
	uint16_t coordinator : 1;
	uint16_t reserved : 1;
	/* Battery Life Extension */
	uint16_t ble : 1;
	/* Final CAP Slot */
	uint16_t cap_slot : 4;
#endif
} __packed;

struct ieee802154_beacon {
	struct ieee802154_beacon_sf sf;

	/* GTS Fields - Spec is always there */
	struct ieee802154_gts_spec gts;
} __packed;

/* See section 7.3.1 */
struct ieee802154_cmd_assoc_req {
	struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		uint8_t reserved_1 : 1;
		uint8_t dev_type : 1;
		uint8_t power_src : 1;
		uint8_t rx_on : 1;
		uint8_t reserved_2 : 2;
		uint8_t sec_capability : 1;
		uint8_t alloc_addr : 1;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		uint8_t alloc_addr : 1;
		uint8_t sec_capability : 1;
		uint8_t reserved_2 : 2;
		uint8_t rx_on : 1;
		uint8_t power_src : 1;
		uint8_t dev_type : 1;
		uint8_t reserved_1 : 1;
#endif
	} ci;
} __packed;

#define IEEE802154_CMD_ASSOC_REQ_LENGTH 1

/* See section 7.3.2 */
enum ieee802154_association_status_field {
	IEEE802154_ASF_SUCCESSFUL = 0x00,
	IEEE802154_ASF_PAN_AT_CAPACITY = 0x01,
	IEEE802154_ASF_PAN_ACCESS_DENIED = 0x02,
	IEEE802154_ASF_RESERVED = 0x03,
	IEEE802154_ASF_RESERVED_PRIMITIVES = 0x80,
};

struct ieee802154_cmd_assoc_res {
	uint16_t short_addr;
	uint8_t status;
} __packed;

#define IEEE802154_CMD_ASSOC_RES_LENGTH 3

/* See section 7.3.3.2 */
enum ieee802154_disassociation_reason_field {
	IEEE802154_DRF_RESERVED_1 = 0x00,
	IEEE802154_DRF_COORDINATOR_WISH = 0x01,
	IEEE802154_DRF_DEVICE_WISH = 0x02,
	IEEE802154_DRF_RESERVED_2 = 0x03,
	IEEE802154_DRF_RESERVED_PRIMITIVES = 0x80,
};

struct ieee802154_cmd_disassoc_note {
	uint8_t reason;
} __packed;

#define IEEE802154_CMD_DISASSOC_NOTE_LENGTH 1

/* Coordinator realignment, see section 7.3.8 */
struct ieee802154_cmd_coord_realign {
	uint16_t pan_id;
	uint16_t coordinator_short_addr;
	uint8_t channel;
	uint16_t short_addr;
	uint8_t channel_page; /* Optional */
} __packed;

#define IEEE802154_CMD_COORD_REALIGN_LENGTH 3

/* GTS request, see section 7.3.9 */
struct ieee802154_gts_request {
	struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		uint8_t length : 4;
		uint8_t direction : 1;
		uint8_t type : 1;
		uint8_t reserved : 2;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		uint8_t reserved : 2;
		uint8_t type : 1;
		uint8_t direction : 1;
		uint8_t length : 4;
#endif
	} gts;
} __packed;

#define IEEE802154_GTS_REQUEST_LENGTH 1

/* Command Frame Identifiers (CFI), see Section 7.3 */
enum ieee802154_cfi {
	IEEE802154_CFI_UNKNOWN = 0x00,
	IEEE802154_CFI_ASSOCIATION_REQUEST = 0x01,
	IEEE802154_CFI_ASSOCIATION_RESPONSE = 0x02,
	IEEE802154_CFI_DISASSOCIATION_NOTIFICATION = 0x03,
	IEEE802154_CFI_DATA_REQUEST = 0x04,
	IEEE802154_CFI_PAN_ID_CONLICT_NOTIFICATION = 0x05,
	IEEE802154_CFI_ORPHAN_NOTIFICATION = 0x06,
	IEEE802154_CFI_BEACON_REQUEST = 0x07,
	IEEE802154_CFI_COORDINATOR_REALIGNEMENT = 0x08,
	IEEE802154_CFI_GTS_REQUEST = 0x09,
	IEEE802154_CFI_RESERVED = 0x0a,
};

struct ieee802154_command {
	uint8_t cfi;
	union {
		struct ieee802154_cmd_assoc_req assoc_req;
		struct ieee802154_cmd_assoc_res assoc_res;
		struct ieee802154_cmd_disassoc_note disassoc_note;
		struct ieee802154_cmd_coord_realign coord_realign;
		struct ieee802154_gts_request gts_request;
		/* Data request, PAN ID conflict, Orphan notification
		 * or Beacon request do not provide more than the CIF.
		 */
	};
} __packed;

#define IEEE802154_CMD_CFI_LENGTH 1

/** Frame */
struct ieee802154_mpdu {
	struct ieee802154_mhr mhr;
	union {
		void *payload;
		struct ieee802154_beacon *beacon;
		struct ieee802154_command *command;
	};
	struct ieee802154_mfr *mfr;
};

/** Frame build parameters */
struct ieee802154_frame_params {
	struct {
		union {
			uint8_t *ext_addr;
			uint16_t short_addr;
		};

		uint16_t len;
		uint16_t pan_id;
	} dst;

	uint16_t short_addr;
	uint16_t pan_id;
} __packed;

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
struct ieee802154_aux_security_hdr *
ieee802154_validate_aux_security_hdr(uint8_t *buf, uint8_t **p_buf, uint8_t *length);
#endif

struct ieee802154_fcf_seq *ieee802154_validate_fc_seq(uint8_t *buf, uint8_t **p_buf,
						      uint8_t *length);

bool ieee802154_validate_frame(uint8_t *buf, uint8_t length, struct ieee802154_mpdu *mpdu);

uint8_t ieee802154_compute_header_and_authtag_size(struct net_if *iface, struct net_linkaddr *dst,
						   struct net_linkaddr *src);

bool ieee802154_create_data_frame(struct ieee802154_context *ctx, struct net_linkaddr *dst,
				  struct net_linkaddr *src, struct net_buf *buf, uint8_t hdr_len);

struct net_pkt *ieee802154_create_mac_cmd_frame(struct net_if *iface, enum ieee802154_cfi type,
						struct ieee802154_frame_params *params);

void ieee802154_mac_cmd_finalize(struct net_pkt *pkt, enum ieee802154_cfi type);

static inline struct ieee802154_command *ieee802154_get_mac_command(struct net_pkt *pkt)
{
	return (struct ieee802154_command *)(pkt->frags->data + pkt->frags->len);
}

#ifdef CONFIG_NET_L2_IEEE802154_ACK_REPLY
bool ieee802154_create_ack_frame(struct net_if *iface, struct net_pkt *pkt, uint8_t seq);
#endif

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
bool ieee802154_decipher_data_frame(struct net_if *iface, struct net_pkt *pkt,
				    struct ieee802154_mpdu *mpdu);
#else
#define ieee802154_decipher_data_frame(...) true
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */

#endif /* __IEEE802154_FRAME_H__ */
