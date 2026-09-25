/*
 * Copyright (c) 2023 Codecoup
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/fff.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "conn.h"

DEFINE_FAKE_VOID_FUNC(bt_conn_foreach, enum bt_conn_type, bt_conn_foreach_cb, void *);
DEFINE_FAKE_VOID_FUNC(bt_foreach_bond, uint8_t, bt_foreach_bond_cb, void *);

static struct bt_conn_auth_info_cb *bt_auth_info_cb;

struct bt_conn_tmp_str bt_conn_dst_tmp_str(const struct bt_conn *conn)
{
	struct bt_conn_tmp_str val;

	(void)bt_addr_le_to_str(&conn->addr, val.str, sizeof(val.str));

	return val;
}

const bt_addr_le_t *bt_conn_get_dst(const struct bt_conn *conn)
{
	return &conn->addr;
}

bt_security_t bt_conn_get_security(const struct bt_conn *conn)
{
	return conn->info.security.level;
}

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
	ARG_UNUSED(conn);
}

void bt_conn_drop(struct bt_conn **orig)
{
	struct bt_conn *conn = *orig;

	*orig = NULL;
	bt_conn_unref(conn);
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
	conn->info.state = BT_CONN_STATE_CONNECTED;

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->connected) {
			cb->connected(conn, err);
		}
	}
}

void mock_bt_conn_disconnected(struct bt_conn *conn, uint8_t err)
{
	conn->info.state = BT_CONN_STATE_DISCONNECTING;

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->disconnected) {
			cb->disconnected(conn, err);
		}
	}

	conn->info.state = BT_CONN_STATE_DISCONNECTED;
}

#if defined(CONFIG_BT_SMP)
void mock_bt_conn_security_changed(struct bt_conn *conn, bt_security_t level,
				   enum bt_security_err err)
{
	if (err == BT_SECURITY_ERR_SUCCESS) {
		conn->info.security.level = level;
	}

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->security_changed) {
			cb->security_changed(conn, level, err);
		}
	}
}
#endif /* CONFIG_BT_SMP */

bool bt_conn_is_type(const struct bt_conn *conn, enum bt_conn_type type)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(type);

	return true;
}
