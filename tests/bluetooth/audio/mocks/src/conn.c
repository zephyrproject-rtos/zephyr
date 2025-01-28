/*
 * Copyright (c) 2023 Codecoup
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <stdint.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/sys/iterable_sections.h>

#include "conn.h"

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
