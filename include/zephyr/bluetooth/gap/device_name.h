/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_GAP_DEVICE_NAME_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_GAP_DEVICE_NAME_H_

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Maximum size of a Bluetooth GAP Device Name according to Bluetooth
 * specification. See Core Specification vol. 3, part C, 12.1.
 *
 * @note It is recommended to use that value when reading the Bluetooth GAP
 * Device Name from a peer.
 */
#define BT_GAP_DEVICE_NAME_MAX_SIZE 248

/**
 * @brief Copy over Bluetooth GAP Device Name
 *
 * @note If buf is equal to NULL, the function will do nothing but return the
 * size of the current Bluetooth GAP Device Name.
 *
 * @param buf Buffer where the Bluetooth GAP Device Name will be copied
 * @param size Size of @p buf
 *
 * @retval >=0 Size of the GAP Device Name
 * @retval -ENOMEM Device name is bigger than @p buf and thus cannot be copied
 * in it
 */
int bt_gap_get_device_name(uint8_t *buf, size_t size);

/**
 * @brief Set the Bluetooth GAP Device Name
 *
 * If the Bluetooth settings are enabled (@kconfig{CONFIG_BT_SETTINGS}) and the
 * storing the name fail, no update will be made. The previous name will be
 * kept.
 *
 * @note If the dynamic Bluetooth GAP Device Name
 * (@kconfig{CONFIG_BT_GAP_DEVICE_NAME_DYNAMIC}) is not enabled, the function
 * will do nothing an always return 0.
 *
 * @warning This API is considered thread safe only if no other APIs tries to
 * write to the settings key used to store the GAP Device Name.
 *
 * @param buf Buffer containing the new Bluetooth GAP device name
 * @param size Size of @p buf
 *
 * @retval 0 Success
 * @retval -ENOBUFS @p size is bigger than the maximum allowed size
 * @retval -EIO Error while trying to save the device name to the settings
 * backend
 */
int bt_gap_set_device_name(const uint8_t *buf, size_t size);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_GAP_DEVICE_NAME_H_ */
