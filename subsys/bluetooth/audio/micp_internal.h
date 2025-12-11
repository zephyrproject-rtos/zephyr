/** @file
 *  @brief Internal APIs for Bluetooth MICP.
 */

/*
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICP_INTERNAL_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICP_INTERNAL_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/atomic.h>

enum bt_micp_mic_ctlr_flag {
	BT_MICP_MIC_CTLR_FLAG_BUSY,

	BT_MICP_MIC_CTLR_FLAG_NUM_FLAGS, /* keep as last */
};

struct bt_micp_mic_ctlr {
	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t mute_handle;
	struct bt_gatt_subscribe_params mute_sub_params;
	struct bt_gatt_discover_params mute_sub_disc_params;

	uint8_t mute_val_buf[1]; /* Mute value is a single octet */
	struct bt_gatt_write_params write_params;
	struct bt_gatt_read_params read_params;
	struct bt_gatt_discover_params discover_params;
	struct bt_conn *conn;

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
	uint8_t aics_inst_cnt;
	struct bt_aics *aics[CONFIG_BT_MICP_MIC_CTLR_MAX_AICS_INST];
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */

	ATOMIC_DEFINE(flags, BT_MICP_MIC_CTLR_FLAG_NUM_FLAGS);
};

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICP_INTERNAL_ */
