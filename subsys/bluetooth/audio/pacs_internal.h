/* @file
 * @brief Internal APIs for PACS handling
 *
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#define BT_AUDIO_LOCATION_MASK BIT_MASK(28)

struct bt_pac_codec {
	uint8_t  id;			/* Codec ID */
	uint16_t cid;			/* Company ID */
	uint16_t vid;			/* Vendor specific Codec ID */
} __packed;

struct bt_pac_ltv {
	uint8_t  len;
	uint8_t  type;
	FLEXIBLE_ARRAY_DECLARE(uint8_t, value);
} __packed;

struct bt_pac_ltv_data {
	uint8_t  len;
	FLEXIBLE_ARRAY_DECLARE(struct bt_pac_ltv, data);
} __packed;

struct bt_pacs_read_rsp {
	uint8_t  num_pac;		/* Number of PAC Records*/
} __packed;

struct bt_pacs_context {
	uint16_t  snk;
	uint16_t  src;
} __packed;
