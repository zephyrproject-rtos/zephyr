/* btp_vocs.h - Bluetooth tester headers */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#define BTP_VOCS_READ_SUPPORTED_COMMANDS	0x01
struct btp_vocs_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_VOCS_UPDATE_LOC			0x02
struct btp_vocs_audio_loc_cmd {
	bt_addr_le_t address;
	uint32_t loc;
} __packed;

#define BTP_VOCS_UPDATE_DESC			0x03
struct btp_vocs_audio_desc_cmd {
	uint8_t desc_len;
	uint8_t desc[0];
} __packed;

#define BTP_VOCS_STATE_GET			0x04
struct btp_vocs_state_get_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_VOCS_LOCATION_GET			0x05
struct btp_vocs_location_get_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_VOCS_OFFSET_STATE_SET		0x06
struct btp_vocs_offset_set_cmd {
	bt_addr_le_t address;
	int16_t offset;
} __packed;

#define BTP_VOCS_OFFSET_STATE_EV		0x80
struct btp_vocs_offset_state_ev {
	bt_addr_le_t address;
	uint8_t att_status;
	int16_t offset;
} __packed;

#define BTP_VOCS_AUDIO_LOCATION_EV		0x81
struct btp_vocs_audio_location_ev {
	bt_addr_le_t address;
	uint8_t att_status;
	uint32_t location;
} __packed;

#define BTP_VOCS_PROCEDURE_EV			0x82
struct btp_vocs_procedure_ev {
	bt_addr_le_t address;
	uint8_t att_status;
	uint8_t opcode;
} __packed;
