/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IEEE 802.15.4 MAC frame related functions
 *
 * @details This is not to be included by the application.
 *
 * @note All references to the standard in this file cite IEEE 802.15.4-2020.
 *
 * @note All structs and attributes (e.g. PAN id, ext address and short address)
 * in this file that directly represent parts of IEEE 802.15.4 frames are in
 * LITTLE ENDIAN, see section 4, especially section 4.3.
 */

#ifndef __IEEE802154_FRAME_H__
#define __IEEE802154_FRAME_H__

#include <zephyr/kernel.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/toolchain.h>

/**
 * @brief Length in octets of an Immediate Acknowledgment (Imm-Ack) frame MPDU.
 *
 * @details Imm-Ack consists of the Frame Control field and the Sequence Number
 * field only; see section 7.3.3 and Figure 7-17.
 */
#define IEEE802154_ACK_PKT_LENGTH 3

/**
 * @brief Minimum MAC frame NMPDU length supported here (same as @ref IEEE802154_ACK_PKT_LENGTH).
 *
 * @details The shortest standardized MAC frame is the Imm-Ack; see section 7.3.3.
 */
#define IEEE802154_MIN_LENGTH     IEEE802154_ACK_PKT_LENGTH

/**
 * @brief Combined length in octets of the Frame Control and Sequence Number fields.
 *
 * @details These are the first three octets of the general MAC frame format;
 * see Figure 7-2 and section 7.2.2.
 */
#define IEEE802154_FCF_SEQ_LENGTH 3

/**
 * @brief Length in octets of a PAN identifier in the MAC addressing fields.
 *
 * @details The PAN ID is two octets; see section 7.2.2.8.
 */
#define IEEE802154_PAN_ID_LENGTH  2

/**
 * @brief Minimum length in octets of the Beacon Payload field.
 *
 * @details Includes the Superframe Specification, GTS Specification, and
 * Pending Address Specification fields at minimum; see section 7.3.1 and Figure 7-6.
 */
#define IEEE802154_BEACON_MIN_SIZE        4

/**
 * @brief Length in octets of the Superframe Specification field.
 *
 * @details See section 7.3.1.4 and Figure 7-7.
 */
#define IEEE802154_BEACON_SF_SIZE         2

/**
 * @brief Length in octets of the GTS Specification field.
 *
 * @details See section 7.3.1.5 and Figure 7-9.
 */
#define IEEE802154_BEACON_GTS_SPEC_SIZE   1

/**
 * @brief Minimum length in octets of the GTS Fields when only the GTS Specification is present.
 *
 * @details Same as @ref IEEE802154_BEACON_GTS_SPEC_SIZE when there are no GTS descriptors.
 */
#define IEEE802154_BEACON_GTS_IF_MIN_SIZE IEEE802154_BEACON_GTS_SPEC_SIZE

/**
 * @brief Length in octets of the Pending Address Specification field.
 *
 * @details See section 7.3.1.6 and Figure 7-12.
 */
#define IEEE802154_BEACON_PAS_SPEC_SIZE   1

/**
 * @brief Minimum length in octets of the Pending Address Fields when only the
 * specification is present.
 *
 * @details Same as @ref IEEE802154_BEACON_PAS_SPEC_SIZE when there are no
 * pending addresses listed.
 */
#define IEEE802154_BEACON_PAS_IF_MIN_SIZE IEEE802154_BEACON_PAS_SPEC_SIZE

/**
 * @brief Length in octets of the GTS Direction field.
 *
 * @details Present when the GTS Descriptor Count subfield is nonzero; see section 7.3.1.5
 * and Figure 7-10.
 */
#define IEEE802154_BEACON_GTS_DIR_SIZE    1

/**
 * @brief Length in octets of one GTS descriptor in the GTS List field.
 *
 * @details Each descriptor is three octets; see section 7.3.1.5 and Figure 7-11.
 */
#define IEEE802154_BEACON_GTS_SIZE        3

/**
 * @brief GTS descriptor direction value for a receive-only GTS.
 *
 * @details See the Device Type subfield in Figure 7-11.
 */
#define IEEE802154_BEACON_GTS_RX          1

/**
 * @brief GTS descriptor direction value for a transmit-only GTS.
 *
 * @details See the Device Type subfield in Figure 7-11.
 */
#define IEEE802154_BEACON_GTS_TX          0

/**
 * @brief Values of the Frame Type subfield in the Frame Control field.
 *
 * @details See section 7.2.2.2 and Table 7-1.
 */
enum ieee802154_frame_type {
	/** Beacon frame. */
	IEEE802154_FRAME_TYPE_BEACON = 0x0,
	/** Data frame. */
	IEEE802154_FRAME_TYPE_DATA = 0x1,
	/** Acknowledgment frame. */
	IEEE802154_FRAME_TYPE_ACK = 0x2,
	/** MAC command frame. */
	IEEE802154_FRAME_TYPE_MAC_COMMAND = 0x3,
	/** Reserved frame type. */
	IEEE802154_FRAME_TYPE_RESERVED = 0x4,
	/** Multipurpose frame. */
	IEEE802154_FRAME_TYPE_MULTIPURPOSE = 0x5,
	/** FRak frame (fragment acknowledgment). */
	IEEE802154_FRAME_TYPE_FRAK = 0x6,
	/** Extended frame type; use the Extended Frame Type field. */
	IEEE802154_FRAME_TYPE_EXTENDED = 0x7,
};

/**
 * @brief Destination and Source Addressing Mode subfield values.
 *
 * @details See section 7.2.2.9 and Table 7-3.
 */
enum ieee802154_addressing_mode {
	/** Addressing fields omitted (not present). */
	IEEE802154_ADDR_MODE_NONE = 0x0,
	/** Reserved; shall not be used. */
	IEEE802154_ADDR_MODE_RESERVED = 0x1,
	/** Short address (16 bit). */
	IEEE802154_ADDR_MODE_SHORT = 0x2,
	/** Extended address (64 bit). */
	IEEE802154_ADDR_MODE_EXTENDED = 0x3,
};

/**
 * @brief Frame Version subfield values.
 *
 * @details See section 7.2.2.10.
 */
enum ieee802154_version {
	/** IEEE Std 802.15.4-2003. */
	IEEE802154_VERSION_802154_2003 = 0x0,
	/** IEEE Std 802.15.4-2006. */
	IEEE802154_VERSION_802154_2006 = 0x1,
	/** IEEE Std 802.15.4 (2003 and 2006 excluded). */
	IEEE802154_VERSION_802154 = 0x2,
	/** Reserved for future use. */
	IEEE802154_VERSION_RESERVED = 0x3,
};

/**
 * @brief Frame Control field and Sequence Number as in the MHR.
 *
 * @details Bit layout follows Figure 7-3; octet order follows section 4.3 (little-endian host).
 * See section 7.2.2.
 */
struct ieee802154_fcf_seq {
	/**
	 * @brief Frame Control field (first two octets of the MHR).
	 *
	 * @details Subfields are as in Figure 7-3.
	 */
	struct {
#ifdef CONFIG_LITTLE_ENDIAN
		/** Frame Type; see @ref ieee802154_frame_type. */
		uint16_t frame_type: 3;
		/** Security Enabled: 1 if the Auxiliary Security Header is present. */
		uint16_t security_enabled: 1;
		/** Frame Pending: more data for the recipient. */
		uint16_t frame_pending: 1;
		/** Acknowledgment Request: sender requests an Imm-Ack or Enh-Ack. */
		uint16_t ar: 1;
		/** PAN ID Compression; see section 7.2.2.5. */
		uint16_t pan_id_comp: 1;
		/** Reserved; shall be zero. */
		uint16_t reserved: 1;
		/** Sequence Number Suppression; no Sequence Number field when set. */
		uint16_t seq_num_suppr: 1;
		/** IE List / Information Elements Present; Header/Payload IEs follow when set. */
		uint16_t ie_list: 1;
		/** Destination Addressing Mode; see @ref ieee802154_addressing_mode. */
		uint16_t dst_addr_mode: 2;
		/** Frame Version; see @ref ieee802154_version. */
		uint16_t frame_version: 2;
		/** Source Addressing Mode; see @ref ieee802154_addressing_mode. */
		uint16_t src_addr_mode: 2;
#else
		/** Reserved; shall be zero. */
		uint16_t reserved: 1;
		/** PAN ID Compression; see section 7.2.2.5. */
		uint16_t pan_id_comp: 1;
		/** Acknowledgment Request: sender requests an Imm-Ack or Enh-Ack. */
		uint16_t ar: 1;
		/** Frame Pending: more data for the recipient. */
		uint16_t frame_pending: 1;
		/** Security Enabled: 1 if the Auxiliary Security Header is present. */
		uint16_t security_enabled: 1;
		/** Frame Type; see @ref ieee802154_frame_type. */
		uint16_t frame_type: 3;
		/** Source Addressing Mode; see @ref ieee802154_addressing_mode. */
		uint16_t src_addr_mode: 2;
		/** Frame Version; see @ref ieee802154_version. */
		uint16_t frame_version: 2;
		/** Destination Addressing Mode; see @ref ieee802154_addressing_mode. */
		uint16_t dst_addr_mode: 2;
		/** IE List / Information Elements Present; Header/Payload IEs follow when set. */
		uint16_t ie_list: 1;
		/** Sequence Number Suppression; no Sequence Number field when set. */
		uint16_t seq_num_suppr: 1;
#endif
	} fc __packed;

	/**
	 * Data Sequence Number (DSN) for outgoing frames, or received sequence
	 * number; see section 7.2.2.4.
	 */
	uint8_t sequence;
} __packed;

/**
 * @brief Short or extended address value in MAC addressing fields.
 *
 * @details Layout matches the Address fields in section 7.2.2; extended address uses a
 * flexible member for the eight octets when present.
 */
struct ieee802154_address {
	/**
	 * @brief Short or extended address in the Address field.
	 *
	 * Mutually exclusive per Addressing Mode; see section 7.2.2.
	 */
	union {
		/** 16-bit short address. */
		uint16_t short_addr;
		/** Start of 64-bit extended address (eight octets). */
		uint8_t ext_addr[0];
	};
} __packed;

/**
 * @brief Destination or Source Address field when PAN ID Compression omits the PAN ID.
 *
 * @details See section 7.2.2.6; only the address is carried in this form.
 */
struct ieee802154_address_field_comp {
	/** Short or extended address when the PAN ID is omitted (compressed). */
	struct ieee802154_address addr;
} __packed;

/**
 * @brief Destination or Source Address field with PAN ID and address.
 *
 * @details See section 7.2.2.6 and Figure 7-4.
 */
struct ieee802154_address_field_plain {
	/** PAN identifier for this address field. */
	uint16_t pan_id;
	/** Short or extended address following the PAN ID. */
	struct ieee802154_address addr;
} __packed;

/**
 * @brief Destination or Source Address field in either compressed or plain form.
 *
 * @details Which variant applies follows from the Frame Control and addressing mode rules
 * in section 7.2.2.
 */
struct ieee802154_address_field {
	/**
	 * @brief Plain or PAN-compressed Destination/Source Address field.
	 *
	 * See section 7.2.2.6.
	 */
	union {
		/** PAN ID and address (plain addressing). */
		struct ieee802154_address_field_plain plain;
		/** Address only (PAN ID compressed). */
		struct ieee802154_address_field_comp comp;
	};
} __packed;

/**
 * @brief Security Level subfield values in the Security Control field.
 *
 * @details See section 9.4.2.2 and Table 9-6.
 */
enum ieee802154_security_level {
	/** No security (no encryption, no authentication). */
	IEEE802154_SECURITY_LEVEL_NONE = 0x0,
	/** MIC-32 authentication only. */
	IEEE802154_SECURITY_LEVEL_MIC_32 = 0x1,
	/** MIC-64 authentication only. */
	IEEE802154_SECURITY_LEVEL_MIC_64 = 0x2,
	/** MIC-128 authentication only. */
	IEEE802154_SECURITY_LEVEL_MIC_128 = 0x3,
	/** Reserved. */
	IEEE802154_SECURITY_LEVEL_RESERVED = 0x4,
	/** Encryption with MIC-32. */
	IEEE802154_SECURITY_LEVEL_ENC_MIC_32 = 0x5,
	/** Encryption with MIC-64. */
	IEEE802154_SECURITY_LEVEL_ENC_MIC_64 = 0x6,
	/** Encryption with MIC-128. */
	IEEE802154_SECURITY_LEVEL_ENC_MIC_128 = 0x7,
};

/**
 * @brief Compare security levels against this to detect encryption modes.
 *
 * @details Security level values strictly above this constant use encryption (ENC MIC);
 * see Table 9-6. Defined as the reserved codepoint @ref IEEE802154_SECURITY_LEVEL_RESERVED.
 */
#define IEEE802154_SECURITY_LEVEL_ENC IEEE802154_SECURITY_LEVEL_RESERVED

/**
 * @brief Authentication tag length in octets for MIC-32 modes.
 *
 * @details Matches @ref IEEE802154_SECURITY_LEVEL_MIC_32 and
 * @ref IEEE802154_SECURITY_LEVEL_ENC_MIC_32.
 */
#define IEEE802154_AUTH_TAG_LENGTH_32  4

/**
 * @brief Authentication tag length in octets for MIC-64 modes.
 *
 * @details Matches @ref IEEE802154_SECURITY_LEVEL_MIC_64 and
 * @ref IEEE802154_SECURITY_LEVEL_ENC_MIC_64.
 */
#define IEEE802154_AUTH_TAG_LENGTH_64  8

/**
 * @brief Authentication tag length in octets for MIC-128 modes.
 *
 * @details Matches @ref IEEE802154_SECURITY_LEVEL_MIC_128 and
 * @ref IEEE802154_SECURITY_LEVEL_ENC_MIC_128.
 */
#define IEEE802154_AUTH_TAG_LENGTH_128 16

/**
 * @brief Key Identifier Mode subfield values.
 *
 * @details See section 9.4.2.3 and Table 9-7.
 */
enum ieee802154_key_id_mode {
	/** Key is determined implicitly. */
	IEEE802154_KEY_ID_MODE_IMPLICIT = 0x0,
	/** Key Index only. */
	IEEE802154_KEY_ID_MODE_INDEX = 0x1,
	/** 4-octet Key Source and Key Index. */
	IEEE802154_KEY_ID_MODE_SRC_4_INDEX = 0x2,
	/** 8-octet Key Source and Key Index. */
	IEEE802154_KEY_ID_MODE_SRC_8_INDEX = 0x3,
};

/**
 * @brief Length in octets of the Key Identifier field for Key Identifier Mode 1.
 *
 * @details Key Index only; see Table 9-7.
 */
#define IEEE802154_KEY_ID_FIELD_INDEX_LENGTH       1

/**
 * @brief Length in octets of the Key Identifier field for Key Identifier Mode 2.
 *
 * @details Four-octet Key Source plus Key Index; see Table 9-7.
 */
#define IEEE802154_KEY_ID_FIELD_SRC_4_INDEX_LENGTH 5

/**
 * @brief Length in octets of the Key Identifier field for Key Identifier Mode 3.
 *
 * @details Eight-octet Key Source plus Key Index; see Table 9-7.
 */
#define IEEE802154_KEY_ID_FIELD_SRC_8_INDEX_LENGTH 9

/**
 * @brief Maximum key length in octets for MAC security operations in this API.
 *
 * @details Aligns with typical AES-128 key sizes; see section 9.4.
 */
#define IEEE802154_KEY_MAX_LEN 16

/**
 * @brief Security Control field (first octet of the Auxiliary Security Header).
 *
 * @details See section 9.4.2 and Figure 9-5.
 */
struct ieee802154_security_control_field {
#ifdef CONFIG_LITTLE_ENDIAN
	/** Security Level; see @ref ieee802154_security_level. */
	uint8_t security_level: 3;
	/** Key Identifier Mode; see @ref ieee802154_key_id_mode. */
	uint8_t key_id_mode: 2;
	/** Reserved; shall be zero. */
	uint8_t reserved: 3;
#else
	/** Reserved; shall be zero. */
	uint8_t reserved: 3;
	/** Key Identifier Mode; see @ref ieee802154_key_id_mode. */
	uint8_t key_id_mode: 2;
	/** Security Level; see @ref ieee802154_security_level. */
	uint8_t security_level: 3;
#endif
} __packed;

/**
 * @brief Length in octets of the Security Control field.
 *
 * @details One octet; see section 9.4.2.
 */
#define IEEE802154_SECURITY_CF_LENGTH 1

/**
 * @brief Key Identifier field of the Auxiliary Security Header.
 *
 * @details Format depends on Key Identifier Mode; see section 9.4.4.
 *
 * @note Currently only mode 0 is supported, so this structure holds no info,
 * yet.
 */
struct ieee802154_key_identifier_field {
	/**
	 * @brief Key Identifier field layout for Modes 1–3.
	 *
	 * See section 9.4.4.
	 */
	union {
		/** Key Identifier Mode 1: Key Index only. */
		struct {
			/** Key Index. */
			uint8_t key_index;
		} mode_1;

		/** Key Identifier Mode 2: 4-octet Key Source and Key Index. */
		struct {
			/** Key Source (four octets). */
			uint8_t key_src[4];
			/** Key Index. */
			uint8_t key_index;
		} mode_2;

		/** Key Identifier Mode 3: 8-octet Key Source and Key Index. */
		struct {
			/** Key Source (eight octets). */
			uint8_t key_src[8];
			/** Key Index. */
			uint8_t key_index;
		} mode_3;
	};
} __packed;

/**
 * @brief Auxiliary Security Header (ASH) following addressing fields when security is enabled.
 *
 * @details See section 9.4 and Figure 9-4.
 */
struct ieee802154_aux_security_hdr {
	/** Security Control field. */
	struct ieee802154_security_control_field control;
	/** Frame Counter field (four octets). */
	uint32_t frame_counter;
	/** Key Identifier field; length depends on Key Identifier Mode. */
	struct ieee802154_key_identifier_field kif;
} __packed;

/**
 * @brief Length in octets of the Frame Counter field in the Auxiliary Security Header.
 *
 * @details Four octets; see section 9.4.3.
 */
#define IEEE802154_SECURITY_FRAME_COUNTER_LENGTH 4

/**
 * @brief Parsed view of the MAC Header (MHR) and Auxiliary Security Header pointers.
 *
 * @details Logical fields of the general MAC frame; see section 7.2.1 and Figure 7-2.
 * Pointers refer into a contiguous PSDU buffer.
 */
struct ieee802154_mhr {
	/** Frame Control and Sequence Number. */
	struct ieee802154_fcf_seq *fs;
	/** Destination address field, if present per Frame Control. */
	struct ieee802154_address_field *dst_addr;
	/** Source address field, if present per Frame Control. */
	struct ieee802154_address_field *src_addr;
	/** Auxiliary Security Header, if Security Enabled is one. */
	struct ieee802154_aux_security_hdr *aux_sec;
};

/**
 * @brief GTS Direction field in the beacon payload.
 *
 * @details See section 7.3.1.5 and Figure 7-10.
 */
struct ieee802154_gts_dir {
#ifdef CONFIG_LITTLE_ENDIAN
	/** Direction mask for GTS descriptors (one bit per descriptor). */
	uint8_t mask: 7;
	/** Reserved; shall be zero. */
	uint8_t reserved: 1;
#else
	/** Reserved; shall be zero. */
	uint8_t reserved: 1;
	/** Direction mask for GTS descriptors (one bit per descriptor). */
	uint8_t mask: 7;
#endif
} __packed;

/**
 * @brief One GTS descriptor in the GTS List field.
 *
 * @details See section 7.3.1.5 and Figure 7-11.
 */
struct ieee802154_gts {
	/** Short address of the device for which the GTS is described. */
	uint16_t short_address;
#ifdef CONFIG_LITTLE_ENDIAN
	/** GTS Starting Slot subfield. */
	uint8_t starting_slot: 4;
	/** GTS Length subfield. */
	uint8_t length: 4;
#else
	/** GTS Length subfield. */
	uint8_t length: 4;
	/** GTS Starting Slot subfield. */
	uint8_t starting_slot: 4;
#endif
} __packed;

/**
 * @brief GTS Specification field of the beacon payload.
 *
 * @details See section 7.3.1.5 and Figure 7-9.
 */
struct ieee802154_gts_spec {
#ifdef CONFIG_LITTLE_ENDIAN
	/** GTS Descriptor Count subfield. */
	uint8_t desc_count: 3;
	/** Reserved; shall be zero. */
	uint8_t reserved: 4;
	/** GTS Permit subfield. */
	uint8_t permit: 1;
#else
	/** GTS Permit subfield. */
	uint8_t permit: 1;
	/** Reserved; shall be zero. */
	uint8_t reserved: 4;
	/** GTS Descriptor Count subfield. */
	uint8_t desc_count: 3;
#endif
} __packed;

/**
 * @brief Pending Address Specification field of the beacon payload.
 *
 * @details See section 7.3.1.6 and Figure 7-13.
 */
struct ieee802154_pas_spec {
#ifdef CONFIG_LITTLE_ENDIAN
	/** Number of Short Addresses Pending subfield. */
	uint8_t nb_sap: 3;
	/** Reserved; shall be zero. */
	uint8_t reserved_1: 1;
	/** Number of Extended Addresses Pending subfield. */
	uint8_t nb_eap: 3;
	/** Reserved; shall be zero. */
	uint8_t reserved_2: 1;
#else
	/** Reserved; shall be zero. */
	uint8_t reserved_1: 1;
	/** Number of Extended Addresses Pending subfield. */
	uint8_t nb_eap: 3;
	/** Reserved; shall be zero. */
	uint8_t reserved_2: 1;
	/** Number of Short Addresses Pending subfield. */
	uint8_t nb_sap: 3;
#endif
} __packed;

/**
 * @brief Superframe Specification field of the beacon payload.
 *
 * @details See section 7.3.1.4 and Figure 7-7.
 */
struct ieee802154_beacon_sf {
#ifdef CONFIG_LITTLE_ENDIAN
	/** Beacon Order subfield. */
	uint16_t bc_order: 4;
	/** Superframe Order subfield. */
	uint16_t sf_order: 4;
	/** Final CAP Slot subfield. */
	uint16_t cap_slot: 4;
	/** Battery Life Extension subfield. */
	uint16_t ble: 1;
	/** Reserved; shall be zero. */
	uint16_t reserved: 1;
	/** PAN Coordinator subfield. */
	uint16_t coordinator: 1;
	/** Association Permit subfield. */
	uint16_t association: 1;
#else
	/** Superframe Order subfield. */
	uint16_t sf_order: 4;
	/** Beacon Order subfield. */
	uint16_t bc_order: 4;
	/** Association Permit subfield. */
	uint16_t association: 1;
	/** PAN Coordinator subfield. */
	uint16_t coordinator: 1;
	/** Reserved; shall be zero. */
	uint16_t reserved: 1;
	/** Battery Life Extension subfield. */
	uint16_t ble: 1;
	/** Final CAP Slot subfield. */
	uint16_t cap_slot: 4;
#endif
} __packed;

/**
 * @brief Fixed part of the Beacon Payload (Superframe Specification and GTS Specification).
 *
 * @details Additional GTS List, GTS Direction, and Pending Address fields follow per section 7.3.1;
 * see Figure 7-6.
 */
struct ieee802154_beacon {
	/** Superframe Specification field. */
	struct ieee802154_beacon_sf sf;

	/** GTS Specification field (always present in this layout). */
	struct ieee802154_gts_spec gts;
} __packed;

/**
 * @brief Association Request command payload (Capability Information field).
 *
 * @details See section 7.5.2 and Figure 7-22.
 */
struct ieee802154_cmd_assoc_req {
	/** Capability Information bit field. */
	struct {
#ifdef CONFIG_LITTLE_ENDIAN
		/** Reserved; shall be zero. */
		uint8_t reserved_1: 1;
		/** Device type (FFD or RFD). */
		uint8_t dev_type: 1;
		/** Power source. */
		uint8_t power_src: 1;
		/** Receiver on when idle. */
		uint8_t rx_on: 1;
		/** Association type. */
		uint8_t association_type: 1;
		/** Reserved; shall be zero. */
		uint8_t reserved_2: 1;
		/** Security capability. */
		uint8_t sec_capability: 1;
		/** Allocate address. */
		uint8_t alloc_addr: 1;
#else
		/** Allocate address. */
		uint8_t alloc_addr: 1;
		/** Security capability. */
		uint8_t sec_capability: 1;
		/** Reserved; shall be zero. */
		uint8_t reserved_2: 1;
		/** Association type. */
		uint8_t association_type: 1;
		/** Receiver on when idle. */
		uint8_t rx_on: 1;
		/** Power source. */
		uint8_t power_src: 1;
		/** Device type (FFD or RFD). */
		uint8_t dev_type: 1;
		/** Reserved; shall be zero. */
		uint8_t reserved_1: 1;
#endif
	} ci;
} __packed;

/**
 * @brief Length in octets of the Association Request command payload (Capability Information).
 *
 * @details One octet; see section 7.5.2.
 */
#define IEEE802154_CMD_ASSOC_REQ_LENGTH 1

/**
 * @brief Association status values in the Association Response command.
 *
 * @details See section 7.5.3.
 */
enum ieee802154_association_status_field {
	/** Association successful. */
	IEEE802154_ASF_SUCCESSFUL = 0x00,
	/** PAN at capacity. */
	IEEE802154_ASF_PAN_AT_CAPACITY = 0x01,
	/** PAN access denied. */
	IEEE802154_ASF_PAN_ACCESS_DENIED = 0x02,
	/** Reserved. */
	IEEE802154_ASF_RESERVED = 0x03,
	/** Reserved range for MAC primitives. */
	IEEE802154_ASF_RESERVED_PRIMITIVES = 0x80,
};

/**
 * @brief Association Response command payload.
 *
 * @details See section 7.5.3 and Figure 7-23.
 */
struct ieee802154_cmd_assoc_res {
	/** Short address assigned by coordinator, or a special value if none. */
	uint16_t short_addr;
	/** Association status; see @ref ieee802154_association_status_field. */
	uint8_t status;
} __packed;

/**
 * @brief Length in octets of the Association Response command payload.
 *
 * @details Three octets; see section 7.5.3.
 */
#define IEEE802154_CMD_ASSOC_RES_LENGTH 3

/**
 * @brief Disassociation reason codes in the Disassociation Notification command.
 *
 * @details See section 7.5.4.
 */
enum ieee802154_disassociation_reason_field {
	/** Reserved. */
	IEEE802154_DRF_RESERVED_1 = 0x00,
	/** Coordinator wishes device to leave. */
	IEEE802154_DRF_COORDINATOR_WISH = 0x01,
	/** Device wishes to leave. */
	IEEE802154_DRF_DEVICE_WISH = 0x02,
	/** Reserved. */
	IEEE802154_DRF_RESERVED_2 = 0x03,
	/** Reserved range for MAC primitives. */
	IEEE802154_DRF_RESERVED_PRIMITIVES = 0x80,
};

/**
 * @brief Disassociation Notification command payload.
 *
 * @details See section 7.5.4.
 */
struct ieee802154_cmd_disassoc_note {
	/** Disassociation reason; see @ref ieee802154_disassociation_reason_field. */
	uint8_t reason;
} __packed;

/**
 * @brief Length in octets of the Disassociation Notification command payload.
 *
 * @details One octet; see section 7.5.4.
 */
#define IEEE802154_CMD_DISASSOC_NOTE_LENGTH 1

/**
 * @brief Coordinator Realignment command payload.
 *
 * @details See section 7.5.10 and Figure 7-31. Channel Page is optional per the standard.
 */
struct ieee802154_cmd_coord_realign {
	/** PAN identifier. */
	uint16_t pan_id;
	/** Coordinator short address. */
	uint16_t coordinator_short_addr;
	/** Logical channel. */
	uint8_t channel;
	/** Short address of the device addressed by this command. */
	uint16_t short_addr;
	/** Channel page (optional fourth octet of the channel specification). */
	uint8_t channel_page;
} __packed;

/**
 * @brief Minimum length in octets of the Coordinator Realignment command payload covered here.
 *
 * @details First three octets of the payload; see section 7.5.10.
 */
#define IEEE802154_CMD_COORD_REALIGN_LENGTH 3

/**
 * @brief GTS Request command payload.
 *
 * @details See section 7.5.11 and Figure 7-32.
 */
struct ieee802154_gts_request {
	/** GTS request descriptor. */
	struct {
#ifdef CONFIG_LITTLE_ENDIAN
		/** GTS length. */
		uint8_t length: 4;
		/** GTS direction (receive or transmit). */
		uint8_t direction: 1;
		/** Characteristics type. */
		uint8_t type: 1;
		/** Reserved; shall be zero. */
		uint8_t reserved: 2;
#else
		/** Reserved; shall be zero. */
		uint8_t reserved: 2;
		/** Characteristics type. */
		uint8_t type: 1;
		/** GTS direction (receive or transmit). */
		uint8_t direction: 1;
		/** GTS length. */
		uint8_t length: 4;
#endif
	} gts;
} __packed;

/**
 * @brief Length in octets of the GTS Request command payload.
 *
 * @details One octet; see section 7.5.11.
 */
#define IEEE802154_GTS_REQUEST_LENGTH 1

/**
 * @brief Command Frame Identifier (CFI) values for MAC commands.
 *
 * @details See section 7.5.1.
 */
enum ieee802154_cfi {
	/** Unknown or unspecified command. */
	IEEE802154_CFI_UNKNOWN = 0x00,
	/** Association request. */
	IEEE802154_CFI_ASSOCIATION_REQUEST = 0x01,
	/** Association response. */
	IEEE802154_CFI_ASSOCIATION_RESPONSE = 0x02,
	/** Disassociation notification. */
	IEEE802154_CFI_DISASSOCIATION_NOTIFICATION = 0x03,
	/** Data request. */
	IEEE802154_CFI_DATA_REQUEST = 0x04,
	/** PAN ID conflict notification. */
	IEEE802154_CFI_PAN_ID_CONFLICT_NOTIFICATION = 0x05,
	/** Orphan notification. */
	IEEE802154_CFI_ORPHAN_NOTIFICATION = 0x06,
	/** Beacon request. */
	IEEE802154_CFI_BEACON_REQUEST = 0x07,
	/** Coordinator realignment. */
	IEEE802154_CFI_COORDINATOR_REALIGNEMENT = 0x08,
	/** GTS request. */
	IEEE802154_CFI_GTS_REQUEST = 0x09,
	/** Reserved. */
	IEEE802154_CFI_RESERVED = 0x0a,
};

/**
 * @brief Parsed MAC command frame payload.
 *
 * @details The Command Payload follows the Command Frame Identifier; see section 7.5.
 * Data Request, PAN ID Conflict Notification, Orphan Notification, and Beacon Request
 * carry no additional payload beyond the CFI.
 */
struct ieee802154_command {
	/** Command Frame Identifier; see @ref ieee802154_cfi. */
	uint8_t cfi;
	/**
	 * @brief Command-specific payload for the selected command identifier.
	 *
	 * See section 7.5.
	 */
	union {
		/** Association Request command. */
		struct ieee802154_cmd_assoc_req assoc_req;
		/** Association Response command. */
		struct ieee802154_cmd_assoc_res assoc_res;
		/** Disassociation Notification command. */
		struct ieee802154_cmd_disassoc_note disassoc_note;
		/** Coordinator Realignment command. */
		struct ieee802154_cmd_coord_realign coord_realign;
		/** GTS Request command. */
		struct ieee802154_gts_request gts_request;
	};
} __packed;

/**
 * @brief Length in octets of the Command Frame Identifier field.
 *
 * @details One octet; see section 7.5.1.
 */
#define IEEE802154_CMD_CFI_LENGTH 1

/**
 * @brief Parsed MAC Service Data Unit (MSDU) with MHR pointers and payload view.
 *
 * @details Used when inspecting or building frames; payload type depends on Frame Type.
 */
struct ieee802154_mpdu {
	/** MAC header (and optional Auxiliary Security Header) references. */
	struct ieee802154_mhr mhr;
	/**
	 * @brief MAC payload view for the current Frame Type.
	 *
	 * See section 7.2.1.
	 */
	union {
		/** Untyped MAC payload (beacon, data, or command body). */
		void *payload;
		/** Beacon payload when the frame is a beacon. */
		struct ieee802154_beacon *beacon;
		/** MAC command payload when the frame is a MAC command. */
		struct ieee802154_command *command;
	};
	/** Length of the MAC payload in octets. */
	uint16_t payload_length;
};

/**
 * @brief Parameters for building a MAC frame (addresses and PAN identifiers).
 *
 * @details Holds destination and local addressing in CPU byte order for stack use;
 * extended addresses are big-endian per wire format where noted.
 */
struct ieee802154_frame_params {
	/** Destination addressing. */
	struct {
		/**
		 * @brief Destination address for frame construction.
		 *
		 * Short or extended form; see section 7.2.2.
		 */
		union {
			/** Extended destination address (big-endian on the air). */
			uint8_t ext_addr[IEEE802154_EXT_ADDR_LENGTH];
			/** Short destination address (CPU byte order). */
			uint16_t short_addr;
		};

		/** Length of the destination address field in use. */
		uint16_t len;
		/** Destination PAN identifier (CPU byte order). */
		uint16_t pan_id;
	} dst;

	/** Local short address (CPU byte order). */
	uint16_t short_addr;
	/** Source PAN identifier (CPU byte order). */
	uint16_t pan_id;
} __packed;

/** @cond INTERNAL_HIDDEN */

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
struct ieee802154_aux_security_hdr *
ieee802154_validate_aux_security_hdr(uint8_t *buf, uint8_t **p_buf, uint8_t *length);
#endif

struct ieee802154_fcf_seq *ieee802154_validate_fc_seq(uint8_t *buf, uint8_t **p_buf,
						      uint8_t *length);

/**
 * @brief Calculate the beacon header length.
 *
 * @details Returns the length of the MAC payload without the beacon payload,
 * see section 7.3.1.1, figure 7-5.
 *
 * @param buf pointer to the MAC payload
 * @param length buffer length
 *
 * @retval -EINVAL The header is invalid.
 * @return the length of the beacon header
 */
int ieee802514_beacon_header_length(uint8_t *buf, uint8_t length);

bool ieee802154_validate_frame(uint8_t *buf, uint8_t length, struct ieee802154_mpdu *mpdu);

void ieee802154_compute_header_and_authtag_len(struct net_if *iface, struct net_linkaddr *dst,
					       struct net_linkaddr *src, uint8_t *ll_hdr_len,
					       uint8_t *authtag_len);

bool ieee802154_create_data_frame(struct ieee802154_context *ctx, struct net_linkaddr *dst,
				  struct net_linkaddr *src, struct net_buf *buf,
				  uint8_t ll_hdr_len);

struct net_pkt *ieee802154_create_mac_cmd_frame(struct net_if *iface, enum ieee802154_cfi type,
						struct ieee802154_frame_params *params);

void ieee802154_mac_cmd_finalize(struct net_pkt *pkt, enum ieee802154_cfi type);

static inline struct ieee802154_command *ieee802154_get_mac_command(struct net_pkt *pkt)
{
	return (struct ieee802154_command *)(pkt->frags->data + pkt->frags->len);
}

bool ieee802154_create_ack_frame(struct net_if *iface, struct net_pkt *pkt, uint8_t seq);

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
bool ieee802154_decipher_data_frame(struct net_if *iface, struct net_pkt *pkt,
				    struct ieee802154_mpdu *mpdu);
#else
#define ieee802154_decipher_data_frame(...) true
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */

/** INTERNAL_HIDDEN @endcond */

#endif /* __IEEE802154_FRAME_H__ */
