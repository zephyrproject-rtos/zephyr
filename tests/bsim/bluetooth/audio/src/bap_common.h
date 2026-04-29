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

#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>

#include "common.h"

#define LONG_META 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, \
		  0x08U, 0x09U, 0x0aU, 0x0bU, 0x0cU, 0x0dU, 0x0eU, 0x0fU, \
		  0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U, \
		  0x18U, 0x19U, 0x1aU, 0x1bU, 0x1cU, 0x1dU, 0x1eU, 0x1fU, \
		  0x20U, 0x21U, 0x22U, 0x23U, 0x24U, 0x25U, 0x26U, 0x27U, \
		  0x28U, 0x29U, 0x2aU, 0x2bU, 0x2cU, 0x2dU, 0x2eU, 0x2fU, \
		  0x30U, 0x31U, 0x32U, 0x33U, 0x34U, 0x35U, 0x36U, 0x37U, \
		  0x38U, 0x39U, 0x3aU, 0x3bU, 0x3cU, 0x3dU, 0x3eU, 0x3fU, \
		  0x40U, 0x41U, 0x42U, 0x43U, 0x44U, 0x45U, 0x46U, 0x47U, \
		  0x48U, 0x49U, 0x4aU, 0x4bU, 0x4cU, 0x4dU, 0x4eU, 0x4fU, \
		  0x50U, 0x51U, 0x52U, 0x53U, 0x54U, 0x55U, 0x56U, 0x57U, \
		  0x58U, 0x59U, 0x5aU, 0x5bU, 0x5cU, 0x5dU, 0x5eU, 0x5fU, \
		  0x60U, 0x61U, 0x62U, 0x63U, 0x64U, 0x65U, 0x66U, 0x67U, \
		  0x68U, 0x69U, 0x6aU, 0x6bU, 0x6cU, 0x6dU, 0x6eU, 0x6fU, \
		  0x70U, 0x71U, 0x72U, 0x73U, 0x74U, 0x75U, 0x76U, 0x77U, \
		  0x78U, 0x79U, 0x7aU, 0x7bU, 0x7cU, 0x7dU, 0x7eU, 0x7fU, \
		  0x80U, 0x81U, 0x82U, 0x83U, 0x84U, 0x85U, 0x86U, 0x87U

#define LONG_META_LEN (sizeof((uint8_t []){LONG_META}) + 1U) /* Size of data + type */

#define BROADCAST_CODE                                                                             \
	((uint8_t[]){0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U, 0x88U, 0x99U, 0xaaU, 0xbbU, 0xccU, \
		     0xddU, 0xeeU, 0xffU})
#define INCORRECT_BROADCAST_CODE                                                                   \
	((uint8_t[]){0xDEU, 0xADU, 0xBEU, 0xEFU, 0x44U, 0x55U, 0x66U, 0x77U, 0x88U, 0x99U, 0xaaU, 0xbbU, 0xccU, \
		     0xddU, 0xeeU, 0xffU})
#define BAD_BROADCAST_CODE                                                                         \
	((uint8_t[]){0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, \
		     0xFFU, 0xFFU, 0xFFU})

#define BAP_RETRY_WAIT K_MSEC(100)

struct unicast_stream {
	struct audio_test_stream stream;
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
bool bap_stream_is_streaming(const struct bt_bap_stream *bap_stream);
bool cap_stream_is_streaming(const struct bt_cap_stream *cap_stream);
bool audio_test_stream_is_streaming(const struct audio_test_stream *test_stream);
#endif /* ZEPHYR_TEST_BSIM_BT_AUDIO_TEST_COMMON_ */
