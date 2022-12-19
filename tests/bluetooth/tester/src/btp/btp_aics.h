/* aics.h - Bluetooth tester headers */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/sys/util.h>

#define AICS_READ_SUPPORTED_COMMANDS		0x01

#define AICS_SET_GAIN			0x65
#define AICS_MUTE				0x66
#define AICS_UNMUTE				0x67
#define AICS_MAN_GAIN			0x68
#define AICS_AUTO_GAIN			0x69
#define AICS_MAN_GAIN_ONLY		0x6a
#define AICS_AUTO_GAIN_ONLY		0x6b
#define AICS_DESCRIPTION		0x6c

struct aics_set_gain {
	uint32_t gain;
} __packed;
