/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>
#include <zephyr/bluetooth/addr.h>
#include <host/conn_internal.h>

typedef void (*bt_conn_foreach_cb) (struct bt_conn *conn, void *data);

/* List of fakes used by this unit tester */
#define CONN_FFF_FAKES_LIST(FAKE)       \
		FAKE(bt_conn_foreach)           \
		FAKE(bt_conn_get_dst)           \

DECLARE_FAKE_VOID_FUNC(bt_conn_foreach, int, bt_conn_foreach_cb, void *);
DECLARE_FAKE_VALUE_FUNC(const bt_addr_le_t *, bt_conn_get_dst, const struct bt_conn *);
