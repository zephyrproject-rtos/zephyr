/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_SECURITY_H_
#define ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_SECURITY_H_

#include <zephyr/bluetooth/conn.h>

int bt_testlib_secure(struct bt_conn *conn, bt_security_t new_minimum);

#endif /* ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_SECURITY_H_ */
