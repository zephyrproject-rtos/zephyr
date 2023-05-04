/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/conn.h>

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
