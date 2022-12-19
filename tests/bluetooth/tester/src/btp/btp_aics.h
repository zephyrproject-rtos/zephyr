/* btp_aics.h - Bluetooth tester headers */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#define AICS_READ_SUPPORTED_COMMANDS    0x01

#define AICS_SET_GAIN                   0x02
#define AICS_MUTE                       0x03
#define AICS_UNMUTE                     0x04
#define AICS_MAN_GAIN                   0x05
#define AICS_AUTO_GAIN                  0x06
#define AICS_MAN_GAIN_ONLY              0x07
#define AICS_AUTO_GAIN_ONLY             0x08
#define AICS_DESCRIPTION                0x09
#define AICS_MUTE_DISABLE               0x0a

struct aics_set_gain {
	int8_t gain;
} __packed;

struct aics_audio_desc {
	uint8_t desc_len;
	uint8_t desc[0];
} __packed;
