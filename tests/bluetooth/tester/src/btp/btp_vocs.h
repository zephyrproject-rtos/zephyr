/* btp_vocs.h - Bluetooth tester headers */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#define BTP_VOCS_READ_SUPPORTED_COMMANDS    0x01

#define BTP_VOCS_UPDATE_LOC                 0x02
#define BTP_VOCS_UPDATE_DESC                0x03

struct btp_vocs_audio_desc {
	uint8_t desc_len;
	uint8_t desc[0];
} __packed;

struct btp_vocs_audio_loc {
	uint32_t loc;
} __packed;
