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

struct bt_pac_codec_capability {
	uint8_t  len;			/* Codec Capability length */
	uint8_t  type;			/* Codec Capability type */
	uint8_t  data[0];		/* Codec Capability data */
} __packed;

struct bt_pac_meta {
	uint8_t  len;			/* Metadata Length */
	uint8_t  value[0];		/* Metadata Value */
} __packed;

struct bt_pac {
	struct bt_pac_codec codec;	/* Codec ID */
	uint8_t  cc_len;		/* Codec Capabilities Length */
	struct bt_pac_codec_capability cc[0]; /* Codec Specific Capabilities */
	struct bt_pac_meta meta[0];	/* Metadata */
} __packed;

struct bt_pacs_read_rsp {
	uint8_t  num_pac;		/* Number of PAC Records*/
	struct bt_pac pac[0];
} __packed;

struct bt_pacs_context {
	uint16_t  snk;
	uint16_t  src;
} __packed;

void bt_pacs_add_capability(enum bt_audio_dir dir);
void bt_pacs_remove_capability(enum bt_audio_dir dir);
