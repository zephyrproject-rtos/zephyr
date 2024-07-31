/*  Bluetooth Audio Content Control Identifier */

/*
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CCID_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CCID_H_

#include <stdint.h>

#include <zephyr/bluetooth/gatt.h>

/**
 * @brief Allocates a CCID value.
 *
 * This should always be called right before registering a GATT service that contains a
 * @ref BT_UUID_CCID characteristic. Allocating a CCID without registering the characteristic
 * may (in very rare cases) result in duplicated CCIDs on the device.
 *
 * @retval ccid 8-bit unsigned CCID value on success
 * @retval -ENOMEM if no more CCIDs can be allocated
 */
int bt_ccid_alloc_value(void);

/**
 * @brief Get the GATT attribute of a CCID value
 *
 * Searches the current GATT database for a CCID characteristic that has the supplied CCID value.
 *
 * @param ccid The CCID the search for
 *
 * @retval NULL if none was found
 * @retval A pointer to a GATT attribute if found
 */
const struct bt_gatt_attr *bt_ccid_find_attr(uint8_t ccid);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CCID_H_ */
