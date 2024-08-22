/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_ADDR_H_
#define ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_ADDR_H_

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/byteorder.h>

/** Bluetooth LE static random address */
#define BT_TESTLIB_ADDR_LE_RANDOM_C0_00_00_00_00_(last)                                            \
	((bt_addr_le_t){                                                                           \
		.type = BT_ADDR_LE_RANDOM,                                                         \
		.a = {{last, 0x00, 0x00, 0x00, 0x00, 0xc0}},                                       \
	})

#endif /* ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_ADDR_H_ */
