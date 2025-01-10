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
