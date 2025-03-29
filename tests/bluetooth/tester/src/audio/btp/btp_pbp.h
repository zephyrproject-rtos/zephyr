/* btp_pbp.c - Bluetooth PBP Tester */

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This is main.h */

#define BTP_PBP_READ_SUPPORTED_COMMANDS	0x01
struct btp_pbp_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_PBP_SET_PUBLIC_BROADCAST_ANNOUNCEMENT 0x02
struct btp_pbp_set_public_broadcast_announcement_cmd {
	uint8_t features;
	uint8_t metadata_len;
	uint8_t metadata[];
} __packed;

#define BTP_PBP_SET_BROADCAST_NAME 0x03
struct btp_pbp_set_broadcast_name_cmd {
	uint8_t name_len;
	uint8_t name[];
} __packed;

#define BTP_PBP_BROADCAST_SCAN_START 0x04
struct btp_pbp_broadcast_scan_start_cmd {
} __packed;

#define BTP_PBP_BROADCAST_SCAN_STOP 0x05
struct btp_pbp_broadcast_scan_stop_cmd {
} __packed;

#define BTP_PBP_EV_PUBLIC_BROADCAST_ANNOUNCEMENT_FOUND 0x80
struct btp_pbp_ev_public_broadcast_announcement_found_ev {
	bt_addr_le_t address;
	uint8_t broadcast_id[BT_AUDIO_BROADCAST_ID_SIZE];
	uint8_t advertiser_sid;
	uint16_t padv_interval;
	uint8_t pba_features;
	uint8_t broadcast_name_len;
	uint8_t broadcast_name[];
} __packed;
