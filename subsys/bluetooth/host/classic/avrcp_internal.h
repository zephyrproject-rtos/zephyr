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

typedef enum __packed {
	BT_AVRCP_CTYPE_CONTROL = 0x0,
	BT_AVRCP_CTYPE_STATUS = 0x1,
	BT_AVRCP_CTYPE_SPECIFIC_INQUIRY = 0x2,
	BT_AVRCP_CTYPE_NOTIFY = 0x3,
	BT_AVRCP_CTYPE_GENERAL_INQUIRY = 0x4,
	BT_AVRCP_CTYPE_NOT_IMPLEMENTED = 0x8,
	BT_AVRCP_CTYPE_ACCEPTED = 0x9,
	BT_AVRCP_CTYPE_REJECTED = 0xA,
	BT_AVRCP_CTYPE_IN_TRANSITION = 0xB,
	BT_AVRCP_CTYPE_IMPLEMENTED_STABLE = 0xC,
	BT_AVRCP_CTYPE_CHANGED = 0xD,
	BT_AVRCP_CTYPE_INTERIM = 0xF,
} bt_avrcp_ctype_t;

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
	uint8_t ctype: 4;        /* Command type codes */
	uint8_t rfa: 4;          /* Zero according to AV/C command frame */
	uint8_t subunit_id: 3;   /* Zero (0x0) or Ignore (0x7) according to AVRCP */
	uint8_t subunit_type: 5; /* Unit (0x1F) or Panel (0x9) according to AVRCP */
	uint8_t opcode;          /* Unit Info, Subunit Info, Vendor Dependent, or Pass Through */
} __packed;

struct bt_avrcp_unit_info_cmd {
	struct bt_avrcp_header hdr;
	uint8_t data[0];
} __packed;

int bt_avrcp_init(void);
