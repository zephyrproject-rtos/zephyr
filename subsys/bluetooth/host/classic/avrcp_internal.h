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

#define AVRCP_CAT_1                       BIT(0) /* Player/Recorder */
#define AVRCP_CAT_2                       BIT(1) /* Monitor/Amplifier */
#define AVRCP_CAT_3                       BIT(2) /* Tuner */
#define AVRCP_CAT_4                       BIT(3) /* Menu */
#define AVRCP_PLAYER_APPLICATION_SETTINGS BIT(4) /* Bit 0 must also be set */
#define AVRCP_GROUP_NAVIGATION            BIT(5) /* Bit 0 must also be set */
#define AVRCP_BROWSING_SUPPORT            BIT(6)
#define AVRCP_MULTIPLE_MEDIA_PLAYERS      BIT(7)
#define AVRCP_COVER_ART_SUPPORT           BIT(8)

#define AVRCP_SUBUNIT_PAGE              (0) /* Fixed value according to AVRCP */
#define AVRCP_SUBUNIT_EXTENSION_CODE    (7) /* Fixed value according to TA Document 2001012 */
#define BT_AVRCP_UNIT_INFO_CMD_SIZE     (5)
#define BT_AVRCP_UNIT_INFO_RSP_SIZE     (5)
#define BT_AVRCP_SUBUNIT_INFO_RSP_SIZE  (5)
#define BT_AVRCP_SUBUNIT_INFO_CMD_SIZE  (5)
#define BT_AVRCP_FRAGMENT_SIZE          (512)

#define BT_L2CAP_PSM_AVRCP 0x0017
#define BT_L2CAP_PSM_AVRCP_BROWSING 0x001b

#if defined(CONFIG_BT_AVRCP_BROWSING)
#define AVRCP_BROWSING_ENABLE AVRCP_BROWSING_SUPPORT
#else
#define AVRCP_BROWSING_ENABLE 0
#endif /* CONFIG_BT_AVRCP_BROWSING */

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
	BT_AVRVP_PKT_TYPE_SINGLE = 0b00,
	BT_AVRVP_PKT_TYPE_START = 0b01,
	BT_AVRVP_PKT_TYPE_CONTINUE = 0b10,
	BT_AVRVP_PKT_TYPE_END = 0b11,
} bt_avrcp_pkt_type_t;

typedef enum {
	AVRCP_STATE_IDLE,
	AVRCP_STATE_SENDING_CONTINUING,
	AVRCP_STATE_ABORT_CONTINUING,
} avrcp_tg_rsp_state_t;

struct bt_avrcp_ct_frag_reassembly_ctx {
	uint8_t tid;				/**< Transaction ID */
	uint8_t rsp;
	uint16_t total_len;			/**< Total length of complete response */
	uint16_t received_len;			/**< Length already received */
	bool fragmentation_active;		/**< Flag fragmentation is in progress */
};

struct bt_avrcp_tg_tx {
	struct bt_avrcp_tg *tg;
	uint16_t sent_len;
	uint8_t tid;
	uint8_t pdu_id;
	uint8_t rsp;
	avrcp_tg_rsp_state_t state;
} __packed;

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

struct bt_avrcp_avc_pdu {
	uint8_t company_id[BT_AVRCP_COMPANY_ID_SIZE];
	uint8_t pdu_id;
	uint8_t pkt_type; /**< [7:2]: Reserved, [1:0]: Packet Type */
	uint16_t param_len;
	uint8_t param[];
} __packed;

struct bt_avrcp_avc_brow_pdu {
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
