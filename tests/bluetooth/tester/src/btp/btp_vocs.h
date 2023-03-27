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
	uint32_t loc;
} __packed;

#define BTP_VOCS_UPDATE_DESC			0x03
struct btp_vocs_audio_desc_cmd {
	uint8_t desc_len;
	uint8_t desc[0];
} __packed;
