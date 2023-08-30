/* btp_vcp.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#define BTP_VCP_READ_SUPPORTED_COMMANDS		0x01
struct btp_vcp_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_VCP_VOL_CTLR_DISCOVER		0x02
struct btp_vcp_discover_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_VCP_VOL_CTLR_STATE_READ		0x03
struct btp_vcp_state_read_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_VCP_VOL_CTLR_FLAGS_READ		0x04
struct btp_vcp_flags_read_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_VCP_VOL_CTLR_VOL_DOWN		0x05
struct btp_vcp_ctlr_vol_down_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_VCP_VOL_CTLR_VOL_UP			0x06
struct btp_vcp_ctlr_vol_up_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_VCP_VOL_CTLR_UNMUTE_VOL_DOWN	0x07
struct btp_vcp_ctlr_unmute_vol_down_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_VCP_VOL_CTLR_UNMUTE_VOL_UP		0x08
struct btp_vcp_ctlr_unmute_vol_up_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_VCP_VOL_CTLR_SET_VOL		0x09
struct btp_vcp_ctlr_set_vol_cmd {
	bt_addr_le_t address;
	uint8_t volume;
} __packed;

#define BTP_VCP_VOL_CTLR_UNMUTE			0x0a
struct btp_vcp_ctlr_unmute_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_VCP_VOL_CTLR_MUTE			0x0b
struct btp_vcp_ctlr_mute_cmd {
	bt_addr_le_t address;
} __packed;

/* VCP events */
#define BTP_VCP_DISCOVERED_EV			0x80
struct btp_vcp_discovered_ev {
	bt_addr_le_t address;
	uint8_t att_status;
	struct {
		uint16_t control_handle;
		uint16_t flag_handle;
		uint16_t state_handle;
	} vcs_handles;

	struct {
		uint16_t state_handle;
		uint16_t location_handle;
		uint16_t control_handle;
		uint16_t desc_handle;
	} vocs_handles;

	struct {
		uint16_t state_handle;
		uint16_t gain_handle;
		uint16_t type_handle;
		uint16_t status_handle;
		uint16_t control_handle;
		uint16_t desc_handle;
	} aics_handles;
} __packed;

#define BTP_VCP_STATE_EV			0x81
struct btp_vcp_state_ev {
	bt_addr_le_t address;
	uint8_t att_status;
	uint8_t volume;
	uint8_t mute;
} __packed;

#define BTP_VCP_FLAGS_EV			0x82
struct btp_vcp_volume_flags_ev {
	bt_addr_le_t address;
	uint8_t att_status;
	uint8_t flags;
} __packed;

#define BTP_VCP_PROCEDURE_EV			0x83
struct btp_vcp_procedure_ev {
	bt_addr_le_t address;
	uint8_t att_status;
	uint8_t opcode;
} __packed;
