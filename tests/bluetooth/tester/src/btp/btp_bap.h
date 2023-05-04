/* btp_bap.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* BAP commands */
#define BTP_BAP_READ_SUPPORTED_COMMANDS	0x01
struct btp_bap_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_BAP_DISCOVER	0x02
struct btp_bap_discover_cmd {
	bt_addr_le_t address;
} __packed;

#define BTP_BAP_DISCOVERY_STATUS_SUCCESS	0x00
#define BTP_BAP_DISCOVERY_STATUS_FAILED		0x01

/* BAP events */
#define BTP_BAP_EV_DISCOVERY_COMPLETED	0x80
struct btp_bap_discovery_completed_ev {
	bt_addr_le_t address;
	uint8_t status;
} __packed;

#define BTP_BAP_EV_CODEC_CAP_FOUND	0x81
struct btp_bap_codec_cap_found_ev {
	bt_addr_le_t address;
	uint8_t dir;
	uint8_t coding_format;
	uint16_t frequencies;
	uint8_t frame_durations;
	uint32_t octets_per_frame;
} __packed;

#define BTP_BAP_EV_ASE_FOUND	0x82
struct btp_ascs_ase_found_ev {
	bt_addr_le_t address;
	uint8_t dir;
	uint8_t ase_id;
} __packed;
