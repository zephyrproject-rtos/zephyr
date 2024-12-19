/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_GATT_SETTINGS_CLEAR_SRC_COMMON_H_
#define ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_GATT_SETTINGS_CLEAR_SRC_COMMON_H_

#include <zephyr/bluetooth/bluetooth.h>

#define ADVERTISER_NAME "Advertiser Pro II"

#define test_service_uuid                                                                          \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x2e2b8dc3, 0x06e0, 0x4f93, 0x9bb2, 0x734091c356f0))
#define test_characteristic_uuid                                                                   \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x2e2b8dc3, 0x06e0, 0x4f93, 0x9bb2, 0x734091c356f3))

#endif /* ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_GATT_SETTINGS_CLEAR_SRC_COMMON_H_ */
