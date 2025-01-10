/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_GATT_DEVICE_NAME_SRC_COMMON_H_
#define ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_GATT_DEVICE_NAME_SRC_COMMON_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>

#define ADVERTISER_NAME "Advertiser Pro II"

static void generate_name(uint8_t *name, size_t length)
{
	for (size_t i = 0; i < length; i++) {
		name[i] = (i % 26) + 97;
	}
}

#endif /* ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_DEVICE_NAME_CLEAR_SRC_COMMON_H_ */
