/**
 * Common functions and helpers for audio BSIM audio tests
 *
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TEST_BSIM_BT_AUDIO_TEST_COMMON_
#define ZEPHYR_TEST_BSIM_BT_AUDIO_TEST_COMMON_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/bluetooth.h>

#define LONG_META 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, \
		  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, \
		  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, \
		  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, \
		  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, \
		  0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, \
		  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, \
		  0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, \
		  0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, \
		  0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, \
		  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, \
		  0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, \
		  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, \
		  0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, \
		  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, \
		  0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, \
		  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87

#define LONG_META_LEN (sizeof((uint8_t []){LONG_META}) + 1U) /* Size of data + type */

#define BROADCAST_CODE                                                                             \
	((uint8_t[]){0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, \
		     0xdd, 0xee, 0xff})
#define INCORRECT_BROADCAST_CODE                                                                   \
	((uint8_t[]){0xDE, 0xAD, 0xBE, 0xEF, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, \
		     0xdd, 0xee, 0xff})
struct unicast_stream {
	struct bt_cap_stream stream;
	struct bt_audio_codec_cfg codec_cfg;
	struct bt_bap_qos_cfg qos;
};

struct named_lc3_preset {
	const char *name;
	struct bt_bap_lc3_preset preset;
};

extern struct bt_audio_codec_cfg vs_codec_cfg;
extern struct bt_audio_codec_cap vs_codec_cap;

void print_hex(const uint8_t *ptr, size_t len);
void print_codec_cap(const struct bt_audio_codec_cap *codec_cap);
void print_codec_cfg(const struct bt_audio_codec_cfg *codec_cfg);
void print_qos(const struct bt_bap_qos_cfg *qos);
void copy_unicast_stream_preset(struct unicast_stream *stream,
				const struct named_lc3_preset *named_preset);

static inline bool valid_metadata_type(uint8_t type, uint8_t len)
{
	switch (type) {
	case BT_AUDIO_METADATA_TYPE_PREF_CONTEXT:
	case BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT:
		if (len != 2) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_LANG:
		if (len != BT_AUDIO_LANG_SIZE) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_PARENTAL_RATING:
		if (len != 1) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_EXTENDED: /* 1 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_VENDOR:   /* 1 - 255 octets */
		if (len < 1) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_CCID_LIST: /* 2 - 254 octets */
		if (len < 2) {
			return false;
		}

		return true;
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO:     /* 0 - 255 octets */
	case BT_AUDIO_METADATA_TYPE_PROGRAM_INFO_URI: /* 0 - 255 octets */
		return true;
	default:
		return false;
	}
}

#endif /* ZEPHYR_TEST_BSIM_BT_AUDIO_TEST_COMMON_ */
