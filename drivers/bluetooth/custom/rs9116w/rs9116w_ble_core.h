/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RS9116W_BLE_CORE_H_
#define RS9116W_BLE_CORE_H_

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <rsi_common_apis.h>
#include <rsi_bt_common.h>
#include <rsi_bt_common_apis.h>
#include <rsi_ble_apis.h>
#include <rsi_driver.h>
#include "rsi_ble_config.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

extern uint8_t rsi_bt_random_addr[6];

extern atomic_t bt_dev_flags[1];


/* bt_dev flags: the flags defined here represent BT controller state */
enum {
	BT_DEV_ENABLE,
	BT_DEV_READY,
	BT_DEV_PRESET_ID,
	BT_DEV_HAS_PUB_KEY,
	BT_DEV_PUB_KEY_BUSY,

	BT_DEV_SCANNING,
	BT_DEV_EXPLICIT_SCAN,
	BT_DEV_ACTIVE_SCAN,
	BT_DEV_SCAN_FILTER_DUP,
	BT_DEV_SCAN_WL,
	BT_DEV_SCAN_LIMITED,
	BT_DEV_INITIATING,

	BT_DEV_RPA_VALID,

	BT_DEV_ID_PENDING,
	BT_DEV_STORE_ID,

#if defined(CONFIG_BT_BREDR)
	BT_DEV_ISCAN,
	BT_DEV_PSCAN,
	BT_DEV_INQUIRY,
#endif /* CONFIG_BT_BREDR */

	/* Total number of flags - must be at the end of the enum */
	BT_DEV_NUM_FLAGS,
};

void rsi_uuid_convert(const struct bt_uuid *uuid, uuid_t *out);

int bt_conn_init(void);

void bt_gap_process(void);

void bt_gatt_process(void);

void bt_smp_process(void);

void bt_gatt_connected(struct bt_conn *conn);

void bt_gatt_disconnected(struct bt_conn *conn);

void force_rx_evt(void);

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG)
#if !defined(BT_DBG_ENABLED)
#define BT_DBG_ENABLED 1
#endif

#if BT_DBG_ENABLED
#define LOG_LEVEL LOG_LEVEL_DBG
#elif defined(CONFIG_BT_LOG_LEVEL)
#define LOG_LEVEL CONFIG_BT_LOG_LEVEL
#else
#define LOG_LEVEL LOG_LEVEL_NONE
#endif

LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL);

#define BT_DBG(fmt, ...) LOG_DBG(fmt, ##__VA_ARGS__)
#define BT_ERR(fmt, ...) LOG_ERR(fmt, ##__VA_ARGS__)
#define BT_WARN(fmt, ...) LOG_WRN(fmt, ##__VA_ARGS__)
#define BT_INFO(fmt, ...) LOG_INF(fmt, ##__VA_ARGS__)

void rsi_connection_cleanup_task(void);

void rsi_bt_raise_evt(void);

/* NOTE: These helper functions encode into a shared buffer. Therefore, the
 * output must be copied/preserved before making subsequent helper function
 * calls.
 */
const char *bt_hex(const void *buf, size_t len);
const char *bt_addr_str(const bt_addr_t *addr);
const char *bt_addr_le_str(const bt_addr_le_t *addr);
const char *bt_uuid_str(const struct bt_uuid *uuid);

#endif
