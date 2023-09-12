/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RS9116W_BLE_GAP_H_
#define RS9116W_BLE_GAP_H_

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#define LOG_MODULE_NAME rsi_bt_gap
#include "rs9116w_ble_conn.h"

/* Advertisement status enum */
enum {
	/* Advertising set has been created in the host. */
	BT_ADV_CREATED,
	/* Advertising parameters has been set in the controller.
	 * This implies that the advertising set has been created in the
	 * controller.
	 */
	BT_ADV_PARAMS_SET,
	/* Advertising data has been set in the controller. */
	BT_ADV_DATA_SET,
	/* Advertising random address pending to be set in the controller. */
	BT_ADV_RANDOM_ADDR_PENDING,
	/* The private random address of the advertiser is valid for this cycle
	 * of the RPA timeout.
	 */
	BT_ADV_RPA_VALID,
	/* The advertiser set is limited by a timeout, or number of advertising
	 * events, or both.
	 */
	BT_ADV_LIMITED,
	/* Advertiser set is currently advertising in the controller. */
	BT_ADV_ENABLED,
	/* Advertiser should include name in advertising data */
	BT_ADV_INCLUDE_NAME_AD,
	/* Advertiser should include name in scan response data */
	BT_ADV_INCLUDE_NAME_SD,
	/* Advertiser set is connectable */
	BT_ADV_CONNECTABLE,
	/* Advertiser set is scannable */
	BT_ADV_SCANNABLE,
	/* Advertiser set is using extended advertising */
	BT_ADV_EXT_ADV,
	/* Advertiser set has disabled the use of private addresses and is using
	 * the identity address instead.
	 */
	BT_ADV_USE_IDENTITY,
	/* Advertiser has been configured to keep advertising after a connection
	 * has been established as long as there are connections available.
	 */
	BT_ADV_PERSIST,
	/* Advertiser has been temporarily disabled. */
	BT_ADV_PAUSED,
	/* Periodic Advertising has been enabled in the controller. */
	BT_PER_ADV_ENABLED,
	/* Periodic Advertising parameters has been set in the controller. */
	BT_PER_ADV_PARAMS_SET,
	/* Constant Tone Extension parameters for Periodic Advertising
	 * has been set in the controller.
	 */
	BT_PER_ADV_CTE_PARAMS_SET,
	/* Constant Tone Extension for Periodic Advertising has been enabled
	 * in the controller.
	 */
	BT_PER_ADV_CTE_ENABLED,

	BT_ADV_NUM_FLAGS,
};

struct bt_le_ext_adv {
	/* ID Address used for advertising */
	uint8_t id;

	/* Advertising handle */
	uint8_t handle;

	/* Current local Random Address */
	bt_addr_le_t random_addr;

	/* Current target address */
	bt_addr_le_t target_addr;

	ATOMIC_DEFINE(flags, BT_ADV_NUM_FLAGS);
};

#define BT_LE_ADV_CHAN_MAP_CHAN_37              0x01
#define BT_LE_ADV_CHAN_MAP_CHAN_38              0x02
#define BT_LE_ADV_CHAN_MAP_CHAN_39              0x04
#define BT_LE_ADV_CHAN_MAP_ALL                  0x07

#define BT_LE_ADV_FP_NO_WHITELIST               0x00
#define BT_LE_ADV_FP_WHITELIST_SCAN_REQ         0x01
#define BT_LE_ADV_FP_WHITELIST_CONN_IND         0x02
#define BT_LE_ADV_FP_WHITELIST_BOTH             0x03

#define UNDIR_CONN 0x80
#define DIR_CONN 0x81
#define UNDIR_SCAN 0x82
#define UNDIR_NON_CONN 0x83
#define DIR_CONN_LOW_DUTY_CYCLE 0x84


#endif
