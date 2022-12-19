/* vocs.h - Bluetooth tester headers */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/sys/util.h>

#define VOCS_READ_SUPPORTED_COMMANDS	0x01

#define VOCS_UPDATE_AUDIO_LOC				0x65
#define VOCS_AUTO_UPDATE_AUDIO_LOC			0x66
#define VOCS_AUDIO_OUT_DESC_UPDATE			0x67
#define VOCS_AUDIO_AUTO_OUT_DESC_UPDATE		0x68

struct vocs_audio_desc {
	uint8_t desc[8];
} __packed;

struct vocs_audio_loc {
	uint8_t loc[8];
} __packed;



