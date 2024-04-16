/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/conn.h"

#include <zephyr/kernel.h>

DEFINE_FAKE_VOID_FUNC(bt_conn_unref, struct bt_conn *);
DEFINE_FAKE_VALUE_FUNC(int, bt_le_create_conn_cancel);
DEFINE_FAKE_VALUE_FUNC(struct bt_conn *, bt_conn_lookup_state_le, uint8_t, const bt_addr_le_t *,
		       const bt_conn_state_t);
