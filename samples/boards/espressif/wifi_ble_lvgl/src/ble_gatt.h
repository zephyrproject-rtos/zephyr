/*
 * Copyright (c) 2026 NotioNext Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_GATT_H
#define BLE_GATT_H

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

/**
 * @brief Initialize BLE GATT service for WiFi provisioning
 * @return 0 on success, negative error code on failure
 */
int ble_gatt_init(void);

/**
 * @brief Start BLE advertising
 * @return 0 on success, negative error code on failure
 */
int ble_start_advertising(void);

/**
 * @brief Stop BLE advertising
 * @return 0 on success, negative error code on failure
 */
int ble_stop_advertising(void);

/**
 * @brief Send notification to connected client
 * @param data Data to send
 * @param len Length of data
 * @return 0 on success, negative error code on failure
 */
int ble_send_notification(const void *data, uint16_t len);

#endif /* BLE_GATT_H */
