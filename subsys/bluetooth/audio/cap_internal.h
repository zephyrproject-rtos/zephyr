/*  Bluetooth Audio Common Audio Profile internal header */

/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>

bool bt_cap_acceptor_ccid_exist(const struct bt_conn *conn, uint8_t ccid);
