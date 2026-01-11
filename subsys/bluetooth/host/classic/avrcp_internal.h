/** @file
 *  @brief Audio Video Remote Control Profile internal header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (C) 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/classic/avrcp.h>

#define AVCTP_VER_1_4 (0x0104u)
#define AVRCP_VER_1_6 (0x0106u)

#define AVRCP_TG_CAT_1                       BIT(0) /* Player/Recorder */
#define AVRCP_TG_CAT_2                       BIT(1) /* Monitor/Amplifier */
#define AVRCP_TG_CAT_3                       BIT(2) /* Tuner */
#define AVRCP_TG_CAT_4                       BIT(3) /* Menu */
#define AVRCP_TG_PLAYER_APPLICATION_SETTINGS BIT(4) /* Bit 0 must also be set */
#define AVRCP_TG_GROUP_NAVIGATION            BIT(5) /* Bit 0 must also be set */
#define AVRCP_TG_BROWSING_SUPPORT            BIT(6)
#define AVRCP_TG_MULTIPLE_MEDIA_PLAYERS      BIT(7)
#define AVRCP_TG_COVER_ART_SUPPORT           BIT(8)

#define AVRCP_CT_CAT_1                          BIT(0) /* Player/Recorder */
#define AVRCP_CT_CAT_2                          BIT(1) /* Monitor/Amplifier */
#define AVRCP_CT_CAT_3                          BIT(2) /* Tuner */
#define AVRCP_CT_CAT_4                          BIT(3) /* Menu */
#define AVRCP_CT_BROWSING_SUPPORT               BIT(6)
#define AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES BIT(7) /* Cover Art GetImageProperties */
#define AVRCP_CT_COVER_ART_GET_IMAGE            BIT(8) /* Cover Art GetImage */
#define AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL BIT(9) /* Cover Art GetLinkedThumbnail */

#define AVRCP_SUBUNIT_PAGE              (0) /* Fixed value according to AVRCP */
#define AVRCP_SUBUNIT_EXTENSION_CODE    (7) /* Fixed value according to TA Document 2001012 */
#define BT_AVRCP_UNIT_INFO_CMD_SIZE     (5)
#define BT_AVRCP_UNIT_INFO_RSP_SIZE     (5)
#define BT_AVRCP_SUBUNIT_INFO_RSP_SIZE  (5)
#define BT_AVRCP_SUBUNIT_INFO_CMD_SIZE  (5)
#define BT_AVRCP_FRAGMENT_SIZE          (512)

#define BT_L2CAP_PSM_AVRCP 0x0017
#define BT_L2CAP_PSM_AVRCP_BROWSING 0x001b

#define BT_AVRCP_HEADROOM \
	(sizeof(struct bt_l2cap_hdr) + \
	 sizeof(struct bt_avctp_header_start) + \
	 sizeof(struct bt_avrcp_header))

#if defined(CONFIG_BT_AVRCP_BROWSING)
#define AVRCP_CT_BROWSING_ENABLE AVRCP_CT_BROWSING_SUPPORT
#define AVRCP_TG_BROWSING_ENABLE AVRCP_TG_BROWSING_SUPPORT
#else
#define AVRCP_CT_BROWSING_ENABLE 0
#define AVRCP_TG_BROWSING_ENABLE 0
#endif /* CONFIG_BT_AVRCP_BROWSING */

#if defined(CONFIG_BT_AVRCP_TG_COVER_ART)
#define AVRCP_TG_COVER_ART_ENABLE AVRCP_TG_COVER_ART_SUPPORT
#else
#define AVRCP_TG_COVER_ART_ENABLE 0
#endif /* CONFIG_BT_AVRCP_TG_COVER_ART */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES)
#define AVRCP_CT_COVER_ART_FEAT_GET_IMAGE_PROPERTIES_ENABLE AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES
#else
#define AVRCP_CT_COVER_ART_FEAT_GET_IMAGE_PROPERTIES_ENABLE 0
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE_PROPERTIES */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE)
#define AVRCP_CT_COVER_ART_FEAT_GET_IMAGE_ENABLE AVRCP_CT_COVER_ART_GET_IMAGE
#else
#define AVRCP_CT_COVER_ART_FEAT_GET_IMAGE_ENABLE 0
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_IMAGE */

#if defined(CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL)
#define AVRCP_CT_COVER_ART_FEAT_GET_LINKED_THUMBNAIL_ENABLE AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL
#else
#define AVRCP_CT_COVER_ART_FEAT_GET_LINKED_THUMBNAIL_ENABLE 0
#endif /* CONFIG_BT_AVRCP_CT_COVER_ART_GET_LINKED_THUMBNAIL */

#define AVRCP_CT_COVER_ART_FEAT_ENABLE \
	(AVRCP_CT_COVER_ART_FEAT_GET_IMAGE_PROPERTIES_ENABLE | \
	 AVRCP_CT_COVER_ART_FEAT_GET_IMAGE_ENABLE | \
	 AVRCP_CT_COVER_ART_FEAT_GET_LINKED_THUMBNAIL_ENABLE)

typedef enum __packed {
	BT_AVRCP_SUBUNIT_ID_ZERO = 0x0,
	BT_AVRCP_SUBUNIT_ID_IGNORE = 0x7,
} bt_avrcp_subunit_id_t;

typedef enum __packed {
	BT_AVRCP_OPC_VENDOR_DEPENDENT = 0x0,
	BT_AVRCP_OPC_UNIT_INFO = 0x30,
	BT_AVRCP_OPC_SUBUNIT_INFO = 0x31,
	BT_AVRCP_OPC_PASS_THROUGH = 0x7c,
} bt_avrcp_opcode_t;

typedef enum __packed {
	BT_AVRCP_PKT_TYPE_SINGLE = 0b00,
	BT_AVRCP_PKT_TYPE_START = 0b01,
	BT_AVRCP_PKT_TYPE_CONTINUE = 0b10,
	BT_AVRCP_PKT_TYPE_END = 0b11,
} bt_avrcp_pkt_type_t;

typedef enum __packed {
	/** Capabilities */
	BT_AVRCP_PDU_ID_GET_CAPS = 0x10,

	/** Player Application Settings */
	BT_AVRCP_PDU_ID_LIST_PLAYER_APP_SETTING_ATTRS = 0x11,
	BT_AVRCP_PDU_ID_LIST_PLAYER_APP_SETTING_VALS = 0x12,
	BT_AVRCP_PDU_ID_GET_CURR_PLAYER_APP_SETTING_VAL = 0x13,
	BT_AVRCP_PDU_ID_SET_PLAYER_APP_SETTING_VAL = 0x14,
	BT_AVRCP_PDU_ID_GET_PLAYER_APP_SETTING_ATTR_TEXT = 0x15,
	BT_AVRCP_PDU_ID_GET_PLAYER_APP_SETTING_VAL_TEXT = 0x16,
	BT_AVRCP_PDU_ID_INFORM_DISPLAYABLE_CHAR_SET = 0x17,
	BT_AVRCP_PDU_ID_INFORM_BATT_STATUS_OF_CT = 0x18,

	/** Metadata Attributes for Current Media Item */
	BT_AVRCP_PDU_ID_GET_ELEMENT_ATTRS = 0x20,

	/** Notifications */
	BT_AVRCP_PDU_ID_GET_PLAY_STATUS = 0x30,
	BT_AVRCP_PDU_ID_REGISTER_NOTIFICATION = 0x31,
	BT_AVRCP_PDU_ID_EVT_PLAYBACK_STATUS_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVT_TRACK_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVT_TRACK_REACHED_END = 0x31,
	BT_AVRCP_PDU_ID_EVT_TRACK_REACHED_START = 0x31,
	BT_AVRCP_PDU_ID_EVT_PLAYBACK_POS_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVT_BATT_STATUS_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVT_SYSTEM_STATUS_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVT_PLAYER_APP_SETTING_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVT_VOLUME_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVT_ADDRESSED_PLAYER_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVT_AVAILABLE_PLAYERS_CHANGED = 0x31,
	BT_AVRCP_PDU_ID_EVT_UIDS_CHANGED = 0x31,

	/** Continuation */
	BT_AVRCP_PDU_ID_REQ_CONTINUING_RSP = 0x40,
	BT_AVRCP_PDU_ID_ABORT_CONTINUING_RSP = 0x41,

	/** Absolute Volume */
	BT_AVRCP_PDU_ID_SET_ABSOLUTE_VOLUME = 0x50,

	/** Media Player Selection */
	BT_AVRCP_PDU_ID_SET_ADDRESSED_PLAYER = 0x60,

	/** Browsing */
	BT_AVRCP_PDU_ID_SET_BROWSED_PLAYER = 0x70,
	BT_AVRCP_PDU_ID_GET_FOLDER_ITEMS = 0x71,
	BT_AVRCP_PDU_ID_CHANGE_PATH = 0x72,
	BT_AVRCP_PDU_ID_GET_ITEM_ATTRS = 0x73,
	BT_AVRCP_PDU_ID_PLAY_ITEM = 0x74,
	BT_AVRCP_PDU_ID_GET_TOTAL_NUMBER_OF_ITEMS = 0x75,

	/** Search */
	BT_AVRCP_PDU_ID_SEARCH = 0x80,

	/** Now Playing */
	BT_AVRCP_PDU_ID_ADD_TO_NOW_PLAYING = 0x90,

	/** Error Response */
	BT_AVRCP_PDU_ID_GENERAL_REJECT = 0xa0,
} bt_avrcp_pdu_id_t;

/* bt_avrcp_tg flags: the flags defined */
enum {
	BT_AVRCP_TG_FLAG_TX_ONGOING,
	BT_AVRCP_TG_FLAG_ABORT_CONTINUING,    /* AVRCP TG abort continuing is pending */

	/* Total number of flags - must be at the end of the enum */
	BT_AVRCP_TG_NUM_FLAGS,
};

struct bt_avrcp_ct_frag_reassembly_ctx {
	uint8_t tid;
	uint8_t rsp;
	uint8_t pdu_id;
};

struct bt_avrcp_tg_vd_rsp_tx {
	struct bt_avrcp_tg *tg;
	uint16_t sent_len;
	uint8_t tid;
	uint8_t pdu_id;
	uint8_t rsp;

	ATOMIC_DEFINE(flags, BT_AVRCP_TG_NUM_FLAGS);
};

struct bt_avrcp_ct_notify_registration {
	uint8_t tid;
	bool interim_received;
	bt_avrcp_notify_changed_cb_t cb;
};

struct bt_avrcp_tg_notify_state {
	bool registered;   /* CT has an active registration for this event */
	bool interim_sent; /* we have already sent INTERIM */
};

struct bt_avrcp_req {
	uint8_t tid;
	uint8_t subunit;
	uint8_t opcode;
};

struct bt_avrcp_header {
	uint8_t byte0;  /**< [7:4]: RFA, [3:0]: Ctype */
	uint8_t byte1;  /**< [7:3]: Subunit_type, [2:0]: Subunit_ID */
	uint8_t opcode; /**< Unit Info, Subunit Info, Vendor Dependent, or Pass Through */
} __packed;

struct bt_avrcp_avc_vendor_pdu {
	uint8_t company_id[BT_AVRCP_COMPANY_ID_SIZE];
	uint8_t pdu_id;
	uint8_t pkt_type; /**< [7:2]: Reserved, [1:0]: Packet Type */
	uint16_t param_len;
	uint8_t param[];
} __packed;

struct bt_avrcp_brow_pdu {
	uint8_t pdu_id;
	uint16_t param_len;
	uint8_t param[];
} __packed;

/** The 4-bit command type or the 4-bit response code. */
#define BT_AVRCP_HDR_GET_CTYPE_OR_RSP(hdr) FIELD_GET(GENMASK(3, 0), ((hdr)->byte0))
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

/** The Packet Type field qualifies each packet as either start (Packet Type=01), continue
 *  (Packet Type=10), or end packet (Packet Type=11). In the case of a non-fragmented message, this
 *  field (Packet Type=00) simply indicates that the message is sent in a single AV/C frame.
 */
#define BT_AVRCP_AVC_PDU_GET_PACKET_TYPE(pdu) FIELD_GET(GENMASK(1, 0), ((pdu)->pkt_type))

/** The Packet Type field qualifies each packet as either start (Packet Type=01), continue
 *  (Packet Type=10), or end packet (Packet Type=11). In the case of a non-fragmented message, this
 *  field (Packet Type=00) simply indicates that the message is sent in a single AV/C frame.
 */
#define BT_AVRCP_AVC_PDU_SET_PACKET_TYPE(pdu, packet_type)                                         \
	(pdu)->pkt_type = FIELD_PREP(GENMASK(1, 0), (packet_type))

struct bt_avrcp_frame {
	struct bt_avrcp_header hdr;
	uint8_t data[];
} __packed;

int bt_avrcp_init(void);

int bt_avrcp_tg_cover_art_init(uint16_t *psm);

struct bt_avrcp_tg *bt_avrcp_get_tg(struct bt_conn *conn, uint16_t psm);

struct bt_avrcp_ct;

struct bt_conn *bt_avrcp_ct_get_acl_conn(struct bt_avrcp_ct *ct);
