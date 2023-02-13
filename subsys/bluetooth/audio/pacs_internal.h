/* @file
 * @brief Internal APIs for PACS handling
 *
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/audio.h>

#define BT_AUDIO_LOCATION_MASK BIT_MASK(28)

struct bt_pac_codec {
	uint8_t  id;			/* Codec ID */
	uint16_t cid;			/* Company ID */
	uint16_t vid;			/* Vendor specific Codec ID */
} __packed;

/* TODO: Figure out the capabilities types */
#define BT_CODEC_CAP_PARAMS		0x01
#define BT_CODEC_CAP_DRM		0x0a
#define BT_CODEC_CAP_DRM_VALUE		0x0b

struct bt_pac_ltv {
	uint8_t  len;
	uint8_t  type;
	uint8_t  value[0];
} __packed;

struct bt_pac_ltv_data {
	uint8_t  len;
	struct bt_pac_ltv data[0];
} __packed;

struct bt_pacs_read_rsp {
	uint8_t  num_pac;		/* Number of PAC Records*/
} __packed;

struct bt_pacs_context {
	uint16_t  snk;
	uint16_t  src;
} __packed;

bool bt_pacs_context_available(enum bt_audio_dir dir, uint16_t context);
