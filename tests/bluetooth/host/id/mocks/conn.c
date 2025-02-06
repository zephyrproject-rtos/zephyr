/*
 * Copyright (c) 2022-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>

#include "mocks/conn.h"

DEFINE_FAKE_VOID_FUNC(bt_conn_unref, struct bt_conn *);
DEFINE_FAKE_VALUE_FUNC(int, bt_le_create_conn_cancel);
DEFINE_FAKE_VALUE_FUNC(struct bt_conn *, bt_conn_lookup_state_le, uint8_t, const bt_addr_le_t *,
		       const bt_conn_state_t);

bool bt_conn_is_type(const struct bt_conn *conn, enum bt_conn_type type)
{
	return true;
}
