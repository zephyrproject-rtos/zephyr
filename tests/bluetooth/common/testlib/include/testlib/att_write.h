/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_ATT_WRITE_H_
#define ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_ATT_WRITE_H_

#include <stdint.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>

int bt_testlib_att_write(struct bt_conn *conn, enum bt_att_chan_opt bearer, uint16_t handle,
			 const uint8_t *data, uint16_t size);

#endif /* ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_ATT_WRITE_H_ */
