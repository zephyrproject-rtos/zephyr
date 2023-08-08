/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/conn.h>

int bt_testlib_connect(const bt_addr_le_t *peer, struct bt_conn **conn);
