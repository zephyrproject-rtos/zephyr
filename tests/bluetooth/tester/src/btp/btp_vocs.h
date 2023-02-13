/* btp_vocs.h - Bluetooth tester headers */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#define VOCS_READ_SUPPORTED_COMMANDS    0x01

#define VOCS_UPDATE_LOC                 0x02
#define VOCS_UPDATE_DESC                0x03

struct vocs_audio_desc {
	uint8_t desc_len;
	uint8_t desc[0];
} __packed;

struct vocs_audio_loc {
	uint32_t loc;
} __packed;
