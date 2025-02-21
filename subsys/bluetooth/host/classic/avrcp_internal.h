/** @file
 *  @brief Audio Video Remote Control Profile internal header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (C) 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define AVCTP_VER_1_4 (0x0104u)
#define AVRCP_VER_1_6 (0x0106u)

#define AVRCP_CAT_1 BIT(0) /* Player/Recorder */
#define AVRCP_CAT_2 BIT(1) /* Monitor/Amplifier */
#define AVRCP_CAT_3 BIT(2) /* Tuner */
#define AVRCP_CAT_4 BIT(3) /* Menu */

#define AVRCP_SUBUNIT_PAGE           (0) /* Fixed value according to AVRCP */
#define AVRCP_SUBUNIT_EXTENSION_COED (7) /* Fixed value according to TA Document 2001012 */
#define AVRCP_COMPANY_ID_SIZE        (3) /* Length for each company ID */

typedef enum __packed {
	BT_AVRCP_SUBUNIT_ID_ZERO = 0x0,
	BT_AVRCP_SUBUNIT_ID_IGNORE = 0x7,
} bt_avrcp_subunit_id_t;

typedef enum __packed {
	BT_AVRCP_SUBUNIT_TYPE_PANEL = 0x9,
	BT_AVRCP_SUBUNIT_TYPE_UNIT = 0x1F,
} bt_avrcp_subunit_type_t;

typedef enum __packed {
	BT_AVRCP_OPC_VENDOR_DEPENDENT = 0x0,
	BT_AVRCP_OPC_UNIT_INFO = 0x30,
	BT_AVRCP_OPC_SUBUNIT_INFO = 0x31,
	BT_AVRCP_OPC_PASS_THROUGH = 0x7c,
} bt_avrcp_opcode_t;

typedef enum __packed {
	/** Capabilities */
	BT_AVRCP_PDU_ID_GET_CAPABILITIES = 0x10,

	/** Player Application Settings */
	BT_AVRCP_PDU_ID_LIST_PLAYER_APPLCIATION_SETTING_ATTRIBUTES = 0x11,
	BT_AVRCP_PDU_ID_LIST_PLAYER_APPLICATION_SETTING_VALUES = 0x12,
	BT_AVRCP_PDU_ID_GET_CURRENT_PLAYER_APPLICATION_SETTING_VALUE = 0x13,
	BT_AVRCP_PDU_ID_SET_PLAYER_APPLICATION_SETTING_VALUE = 0x14,
	BT_AVRCP_PDU_ID_GET_PLAYER_APPLICATION_SETTING_ATTRIBUTE_TEXT = 0x15,
	BT_AVRCP_PDU_ID_GET_PLAYER_APPLICATION_SETTING_VALUE_TEXT = 0x16,
	BT_AVRCP_PDU_ID_INFORM_DISPLAYABLE_CHARACTER_SET = 0x17,
	BT_AVRCP_PDU_ID_INFORM_BATTERY_STATUS_OF_CT = 0x18,

	/** Metadata Attributes for Current Media Item */
	BT_AVRCP_PDU_ID_GET_ELEMENT_ATTRIBUTES = 0x20,

	/** Notifications */
	BT_AVRCP_PDU_ID_GET_PLAY_STATUS = 0x30,
	BT_AVRCP_PDU_ID_REGISTER_NOTIFICATION = 0x31,
	BT_AVRCP_PDU_ID_EVENT_PLAYBACK_STATUS_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVENT_TRACK_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVENT_TRACK_REACHED_END = 0x31,
	BT_AVRCP_PDU_ID_EVENT_TRACK_REACHED_START = 0x31,
	BT_AVRCP_PDU_ID_EVENT_PLAYBACK_POS_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVENT_BATT_STATUS_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVENT_SYSTEM_STATUS_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVENT_PLAYER_APPLICATION_SETTING_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVENT_VOLUME_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVENT_ADDRESSED_PLAYER_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVENT_AVAILABLE_PLAYERS_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVENT_UIDS_CHANGED = 0x31,

	/** Continuation */
	BT_AVRCP_PDU_ID_REQUEST_CONTINUING_RESPONSE = 0x40,
	BT_AVRCP_PDU_ID_ABORT_CONTINUING_RESPONSE = 0x41,

	/** Absolute Volume */
	BT_AVRCP_PDU_ID_SET_ABSOLUTE_VOLUME = 0x50,

	/** Media Player Selection */
	BT_AVRCP_PDU_ID_SET_ADDRESSED_PLAYER = 0x60,

	/** Browsing */
	BT_AVRCP_PDU_ID_SET_BROWSED_PLAYER = 0x70,
	BT_AVRCP_PDU_ID_GET_FOLDER_ITEMS = 0x71,
	BT_AVRCP_PDU_ID_CHANGE_PATH = 0x72,
	BT_AVRCP_PDU_ID_GET_ITEM_ATTRIBUTES = 0x73,
	BT_AVRCP_PDU_ID_PLAY_ITEM = 0x74,
	BT_AVRCP_PDU_ID_GET_TOTAL_NUMBER_OF_ITEMS = 0x75,

	/** Search */
	BT_AVRCP_PDU_ID_SEARCH = 0x80,

	/** Now Playing */
	BT_AVRCP_PDU_ID_ADD_TO_NOW_PLAYING = 0x90,

	/** Error Response */
	BT_AVRCP_PDU_ID_GENERAL_REJECT = 0xa0,
} bt_avrcp_pdu_id_t;

struct bt_avrcp_req {
	uint8_t tid;
	uint8_t subunit;
	uint8_t opcode;
};

struct bt_avrcp_header {
	uint8_t byte0;  /** [7:4]: RFA, [3:0]: Ctype */
	uint8_t byte1;  /** [7:3]: Subunit_type, [2:0]: Subunit_ID */
	uint8_t opcode; /** Unit Info, Subunit Info, Vendor Dependent, or Pass Through */
} __packed;

/** The 4-bit command type or the 4-bit response code. */
#define BT_AVRCP_HDR_GET_CTYPE_OR_RSP(hdr)        FIELD_GET(GENMASK(3, 0), ((hdr)->byte0))
/** Taken together, the subunit_type and subunit_ID fields define the command recipient’s  address
 *  within the target. These fields enable the target to determine  whether the command is
 *  addressed to the target  unit, or to a specific subunit within the target. The values in these
 *  fields remain unchanged in the response frame.
 */
#define BT_AVRCP_HDR_GET_SUBUNIT_ID(hdr)   FIELD_GET(GENMASK(2, 0), ((hdr)->byte1))
/** Taken together, the subunit_type and subunit_ID fields define the command recipient’s  address
 *  within the target. These fields enable the target to determine  whether the command is
 *  addressed to the target  unit, or to a specific subunit within the target. The values in these
 *  fields remain unchanged in the response frame.
 */
#define BT_AVRCP_HDR_GET_SUBUNIT_TYPE(hdr) FIELD_GET(GENMASK(7, 3), ((hdr)->byte1))

/** The 4-bit command type or the 4-bit response code. */
#define BT_AVRCP_HDR_SET_CTYPE_OR_RSP(hdr, ctype)                                                  \
	(hdr)->byte0 = (((hdr)->byte0) & ~GENMASK(3, 0)) | FIELD_PREP(GENMASK(3, 0), (ctype))
/** Taken together, the subunit_type and subunit_ID fields define the command recipient’s  address
 *  within the target. These fields enable the target to determine  whether the command is
 *  addressed to the target  unit, or to a specific subunit within the target. The values in these
 *  fields remain unchanged in the response frame.
 */
#define BT_AVRCP_HDR_SET_SUBUNIT_ID(hdr, subunit_id)                                               \
	(hdr)->byte1 = (((hdr)->byte1) & ~GENMASK(2, 0)) | FIELD_PREP(GENMASK(2, 0), (subunit_id))
/** Taken together, the subunit_type and subunit_ID fields define the command recipient’s  address
 *  within the target. These fields enable the target to determine  whether the command is
 *  addressed to the target  unit, or to a specific subunit within the target. The values in these
 *  fields remain unchanged in the response frame.
 */
#define BT_AVRCP_HDR_SET_SUBUNIT_TYPE(hdr, subunit_type)                                           \
	(hdr)->byte1 = (((hdr)->byte1) & ~GENMASK(7, 3)) | FIELD_PREP(GENMASK(7, 3), (subunit_type))

struct bt_avrcp_frame {
	struct bt_avrcp_header hdr;
	uint8_t data[0];
} __packed;

int bt_avrcp_init(void);
