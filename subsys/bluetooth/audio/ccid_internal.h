/*  Bluetooth Audio Content Control Identifier */

/*
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CCID_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CCID_H_

#include <zephyr/device.h>
#include <zephyr/types.h>

/**
 * @brief Gets a free CCID value.
 *
 * The maximum number of CCID values that can retrieved on the device is 0xFE,
 * one less than per the GSS specification.
 *
 * @return uint8_t A content control ID value.
 */
uint8_t bt_ccid_get_value(void);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CCID_H_ */
