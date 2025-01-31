/*
 * Copyright (c) 2023 Codecoup
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/fff.h>
#include <zephyr/sys/iterable_sections.h>

#include "conn.h"

DEFINE_FAKE_VOID_FUNC(bt_conn_foreach, enum bt_conn_type, bt_conn_foreach_cb, void *);
DEFINE_FAKE_VALUE_FUNC(const bt_addr_le_t *, bt_conn_get_dst, const struct bt_conn *);
DEFINE_FAKE_VOID_FUNC(bt_foreach_bond, uint8_t, bt_foreach_bond_cb, void *);

static struct bt_conn_auth_info_cb *bt_auth_info_cb;

uint8_t bt_conn_index(const struct bt_conn *conn)
{
	return conn->index;
}

int bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info)
{
	*info = conn->info;

	return 0;
}

struct bt_conn *bt_conn_ref(struct bt_conn *conn)
{
	return conn;
}

void bt_conn_unref(struct bt_conn *conn)
{
}

int bt_conn_auth_info_cb_register(struct bt_conn_auth_info_cb *cb)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	if (bt_auth_info_cb != NULL) {
		return -EALREADY;
	}

	bt_auth_info_cb = cb;

	return 0;
}

void mock_bt_conn_connected(struct bt_conn *conn, uint8_t err)
{
	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->connected) {
			cb->connected(conn, err);
		}
	}
}

void mock_bt_conn_disconnected(struct bt_conn *conn, uint8_t err)
{
	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->disconnected) {
			cb->disconnected(conn, err);
		}
	}
}
