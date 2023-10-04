/* btp_aics.h - Bluetooth tester headers */

/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

/*AICS service */
#define BTP_AICS_READ_SUPPORTED_COMMANDS	0x01
struct btp_aics_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define BTP_AICS_SET_GAIN			0x02
struct btp_aics_set_gain_cmd {
	int8_t gain;
} __packed;

#define BTP_AICS_MUTE				0x03
#define BTP_AICS_UNMUTE				0x04
#define BTP_AICS_MAN_GAIN			0x05
#define BTP_AICS_AUTO_GAIN			0x06
#define BTP_AICS_MAN_GAIN_ONLY			0x07
#define BTP_AICS_AUTO_GAIN_ONLY			0x08

#define BTP_AICS_DESCRIPTION			0x09
struct btp_aics_audio_desc_cmd {
	uint8_t desc_len;
	uint8_t desc[0];
} __packed;

#define BTP_AICS_MUTE_DISABLE			0x0a
