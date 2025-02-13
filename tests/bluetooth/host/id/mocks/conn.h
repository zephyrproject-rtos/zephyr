/*
 * Copyright (c) 2022-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/conn_internal.h>

/* List of fakes used by this unit tester */
#define CONN_FFF_FAKES_LIST(FAKE)                                                                  \
	FAKE(bt_conn_unref)                                                                        \
	FAKE(bt_le_create_conn_cancel)                                                             \
	FAKE(bt_conn_lookup_state_le)

DECLARE_FAKE_VOID_FUNC(bt_conn_unref, struct bt_conn *);
DECLARE_FAKE_VALUE_FUNC(int, bt_le_create_conn_cancel);
DECLARE_FAKE_VALUE_FUNC(struct bt_conn *, bt_conn_lookup_state_le, uint8_t, const bt_addr_le_t *,
			const bt_conn_state_t);
DECLARE_FAKE_VALUE_FUNC(bool, bt_conn_is_type, const struct bt_conn *, enum bt_conn_type);
