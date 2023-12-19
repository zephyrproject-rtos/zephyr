/* btp_has.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Oticon
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/addr.h>

/* HAS commands */
#define BTP_HAS_READ_SUPPORTED_COMMANDS	0x01
struct btp_has_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_HAS_SET_ACTIVE_INDEX	0x02
struct btp_has_set_active_index_cmd {
	bt_addr_le_t address;
	uint8_t index;
} __packed;

#define BTP_HAS_SET_PRESET_NAME		0x03
struct btp_has_set_preset_name_cmd {
	bt_addr_le_t address;
	uint8_t index;
	uint8_t length;
	char    name[0];
} __packed;

#define BTP_HAS_REMOVE_PRESET		0x04
struct btp_has_remove_preset_cmd {
	bt_addr_le_t address;
	uint8_t index;
} __packed;

#define BTP_HAS_ADD_PRESET		0x05
struct btp_has_add_preset_cmd {
	bt_addr_le_t address;
	uint8_t index;
	uint8_t props;
	uint8_t length;
	char    name[0];
} __packed;

#define BTP_HAS_SET_PROPERTIES		0x06
struct btp_has_set_properties_cmd {
	bt_addr_le_t address;
	uint8_t index;
	uint8_t props;
} __packed;

/* HAS events */
#define BTP_HAS_EV_OPERATION_COMPLETED	0x80
struct btp_has_operation_completed_ev {
	bt_addr_le_t address;
	uint8_t index;
	uint8_t opcode;
	uint8_t status;

	/* RFU */
	uint8_t flags;
} __packed;
