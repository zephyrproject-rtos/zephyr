/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_CONN_WAIT_H_
#define ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_CONN_WAIT_H_
#include <zephyr/bluetooth/conn.h>

int bt_testlib_wait_connected(struct bt_conn *conn);
int bt_testlib_wait_disconnected(struct bt_conn *conn);

#endif /* ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_CONN_WAIT_H_ */
