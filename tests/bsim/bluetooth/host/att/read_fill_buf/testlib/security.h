/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/conn.h>

int bt_testlib_secure(struct bt_conn *conn, bt_security_t new_minimum);
