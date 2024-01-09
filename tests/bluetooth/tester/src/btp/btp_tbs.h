/* btp_mics.h - Bluetooth tester headers */

/*
 * Copyright (c) 2024 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* TBS commands */
#define BTP_TBS_READ_SUPPORTED_COMMANDS		0x01
struct btp_tbs_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_TBS_REMOTE_INCOMING			0x02
struct btp_tbs_remote_incoming_cmd {
	uint8_t index;
	uint8_t recv_len;
	uint8_t caller_len;
	uint8_t fn_len;
	uint8_t data_len;
	uint8_t data[0];
} __packed;

#define BTP_TBS_HOLD				0x03
struct btp_tbs_hold_cmd {
	uint8_t index;
} __packed;

#define BTP_TBS_SET_BEARER_NAME			0x04
struct btp_tbs_set_bearer_name_cmd {
	uint8_t index;
	uint8_t name_len;
	uint8_t name[0];
} __packed;

#define BTP_TBS_SET_TECHNOLOGY			0x05
struct btp_tbs_set_technology_cmd {
	uint8_t index;
	uint8_t tech;
} __packed;

#define BTP_TBS_SET_URI_SCHEME			0x06
struct btp_tbs_set_uri_schemes_list_cmd {
	uint8_t index;
	uint8_t uri_len;
	uint8_t uri_count;
	uint8_t uri_list[0];
} __packed;

#define BTP_TBS_SET_STATUS_FLAGS		0x07
struct btp_tbs_set_status_flags_cmd {
	uint8_t index;
	uint16_t flags;
} __packed;

#define BTP_TBS_REMOTE_HOLD			0x08
struct btp_tbs_remote_hold_cmd {
	uint8_t index;
} __packed;

#define BTP_TBS_ORIGINATE			0x09
struct btp_tbs_originate_cmd {
	uint8_t index;
	uint8_t uri_len;
	uint8_t uri[0];
} __packed;

#define BTP_TBS_SET_SIGNAL_STRENGTH		0x0a
struct btp_tbs_set_signal_strength_cmd {
	uint8_t index;
	uint8_t strength;
} __packed;
