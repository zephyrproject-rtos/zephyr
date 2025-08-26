/**
 * @file
 * @brief Bluetooth subsystem core APIs.
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_BLUETOOTH_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_BLUETOOTH_H_

/**
 * @brief Bluetooth APIs
 *
 * @details The Bluetooth Subsystem Core APIs provide essential functionalities
 *          to use and manage Bluetooth based communication. These APIs include
 *          APIs for Bluetooth stack initialization, device discovery,
 *          connection management, data transmission, profiles and services.
 *          These APIs support both classic Bluetooth and Bluetooth Low Energy
 *          (LE) operations.
 *
 * @defgroup bluetooth Bluetooth APIs
 * @ingroup connectivity
 * @{
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/gap.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @typedef bt_ready_cb_t
 * @brief Callback for notifying that Bluetooth has been enabled.
 *
 * @param err zero on success or (negative) error code otherwise.
 */
typedef void (*bt_ready_cb_t)(int err);

/**
 * @brief Enable Bluetooth
 *
 * Enable Bluetooth. Must be the called before any calls that
 * require communication with the local Bluetooth hardware.
 *
 * When @kconfig{CONFIG_BT_SETTINGS} is enabled, the application must load the
 * Bluetooth settings after this API call successfully completes before
 * Bluetooth APIs can be used. Loading the settings before calling this function
 * is insufficient. Bluetooth settings can be loaded with @ref settings_load or
 * @ref settings_load_subtree with argument "bt". The latter selectively loads only
 * Bluetooth settings and is recommended if @ref settings_load has been called
 * earlier.
 *
 * @param cb Callback to notify completion or NULL to perform the
 * enabling synchronously. The callback is called from the system workqueue.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_enable(bt_ready_cb_t cb);

/**
 * @brief Disable Bluetooth
 *
 * Disable Bluetooth. Can't be called before bt_enable has completed.
 *
 * This API will clear all configured identity addresses and keys that are not persistently
 * stored with @kconfig{CONFIG_BT_SETTINGS}. These can be restored
 * with @ref settings_load before reenabling the stack.
 *
 * This API does _not_ clear previously registered callbacks
 * like @ref bt_le_scan_cb_register, @ref bt_conn_cb_register
 * AND @ref bt_br_discovery_cb_register.
 * That is, the application shall not re-register them when
 * the Bluetooth subsystem is re-enabled later.
 *
 * Close and release HCI resources. Result is architecture dependent.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_disable(void);

/**
 * @brief Check if Bluetooth is ready
 *
 * @return true when Bluetooth is ready, false otherwise
 */
bool bt_is_ready(void);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_BLUETOOTH_H_ */
