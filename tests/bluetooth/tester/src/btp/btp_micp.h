/* btp_micp.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* MICP commands */
#define BTP_MICP_READ_SUPPORTED_COMMANDS	0x01
struct btp_micp_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_MICP_CTLR_DISCOVER			0x02
struct btp_micp_discover_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MICP_CTLR_MUTE_READ			0x03
struct btp_micp_mute_read_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_MICP_CTLR_MUTE			0x04
struct btp_micp_mute_cmd {
	bt_addr_le_t address;
} __packed;

/* MICP events */
#define BTP_MICP_DISCOVERED_EV			0x80
struct btp_micp_discovered_ev {
	bt_addr_le_t address;
	uint8_t att_status;
	uint16_t mute_handle;
	uint16_t state_handle;
	uint16_t gain_handle;
	uint16_t type_handle;
	uint16_t status_handle;
	uint16_t control_handle;
	uint16_t desc_handle;
} __packed;

#define BTP_MICP_MUTE_STATE_EV			0x81
struct btp_micp_mute_state_ev {
	bt_addr_le_t address;
	uint8_t att_status;
	uint8_t mute;
} __packed;
