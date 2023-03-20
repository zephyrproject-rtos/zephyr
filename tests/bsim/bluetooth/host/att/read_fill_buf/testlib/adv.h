/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>

int bt_testlib_adv_conn(struct bt_conn **conn, int id, uint32_t adv_options);
