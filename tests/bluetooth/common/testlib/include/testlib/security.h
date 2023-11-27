/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_SECURITY_H_
#define ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_SECURITY_H_

#include <zephyr/bluetooth/conn.h>

int bt_testlib_secure(struct bt_conn *conn, bt_security_t new_minimum);

/** @brief Get LTK for a connection.
 *
 *  This function gets the Long Term Key (LTK) for a connection.
 *
 *  @param conn The connection object.
 *  @param[out] dst Destination buffer for the LTK.
 *
 *  @return 0 on success or negative error
 */

int bt_testlib_get_ltk(const struct bt_conn *conn, uint8_t *dst);

#endif /* ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_SECURITY_H_ */
