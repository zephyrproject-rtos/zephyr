/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>

#include <atomic.h>
#include <misc/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/log.h>

#include "gap_internal.h"
#include "conn_internal.h"

static struct bt_conn conns[CONFIG_BLUETOOTH_MAX_CONN];

static struct bt_conn *conn_new(void)
{
	struct bt_conn *conn = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(conns); i++) {
		if (!atomic_get(&conns[i].ref)) {
			conn = &conns[i];
			break;
		}
	}

	if (!conn) {
		return NULL;
	}

	memset(conn, 0, sizeof(*conn));

	atomic_set(&conn->ref, 1);

	return conn;
}

struct bt_conn *bt_conn_ref(struct bt_conn *conn)
{
	atomic_inc(&conn->ref);

	BT_DBG("handle %u ref %u", conn->handle, atomic_get(&conn->ref));

	return conn;
}

void bt_conn_unref(struct bt_conn *conn)
{
	atomic_dec(&conn->ref);

	BT_DBG("handle %u ref %u", conn->handle, atomic_get(&conn->ref));
}

struct bt_conn *bt_conn_lookup_handle(uint16_t handle)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(conns); i++) {
		if (!atomic_get(&conns[i].ref)) {
			continue;
		}

		if (conns[i].handle == handle) {
			return bt_conn_ref(&conns[i]);
		}
	}

	return NULL;
}

struct bt_conn *bt_conn_lookup_addr_le(const bt_addr_le_t *peer)
{
	return NULL;
}

const bt_addr_le_t *bt_conn_get_dst(const struct bt_conn *conn)
{
	return NULL;
}

int bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info)
{
	return -ENOSYS;
}

int bt_conn_le_param_update(struct bt_conn *conn,
			    const struct bt_le_conn_param *param)
{
	return -ENOSYS;
}

int bt_conn_disconnect(struct bt_conn *conn, uint8_t reason)
{
	return -ENOSYS;
}

struct bt_conn *bt_conn_create_le(const bt_addr_le_t *peer,
				  const struct bt_le_conn_param *param)
{
	return NULL;
}

int bt_conn_security(struct bt_conn *conn, bt_security_t sec)
{
	return -ENOSYS;
}

uint8_t bt_conn_enc_key_size(struct bt_conn *conn)
{
	return 0;
}

void bt_conn_cb_register(struct bt_conn_cb *cb)
{
}

int bt_le_set_auto_conn(bt_addr_le_t *addr,
			const struct bt_le_conn_param *param)
{
	return -ENOSYS;
}

struct bt_conn *bt_conn_create_slave_le(const bt_addr_le_t *peer,
					const struct bt_le_adv_param *param)
{
	return NULL;
}

int bt_conn_auth_cb_register(const struct bt_conn_auth_cb *cb)
{
	return -ENOSYS;
}

int bt_conn_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey)
{
	return -ENOSYS;
}

int bt_conn_auth_cancel(struct bt_conn *conn)
{
	return -ENOSYS;
}

int bt_conn_auth_passkey_confirm(struct bt_conn *conn, bool match)
{
	return -ENOSYS;
}

/* Connection related events */

void on_ble_gap_connect_evt(const struct ble_gap_connect_evt *ev)
{
	struct bt_conn *conn;

	BT_DBG("handle %u", ev->conn_handle);

	conn = conn_new();
	if (!conn) {
		BT_ERR("Unable to create new bt_conn object");
		return;
	}

	conn->handle = ev->conn_handle;
}

void on_ble_gap_disconnect_evt(const struct ble_gap_disconnect_evt *ev)
{
	struct bt_conn *conn;

	conn = bt_conn_lookup_handle(ev->conn_handle);
	if (!conn) {
		BT_ERR("Unable to find conn for handle %u", ev->conn_handle);
		return;
	}

	BT_DBG("conn %p handle %u", conn, ev->conn_handle);

	bt_conn_unref(conn);
	bt_conn_unref(conn);
}

void on_ble_gap_conn_update_evt(const struct ble_gap_conn_update_evt *ev)
{
	struct bt_conn *conn;

	conn = bt_conn_lookup_handle(ev->conn_handle);
	if (!conn) {
		BT_ERR("Unable to find conn for handle %u", ev->conn_handle);
		return;
	}

	BT_DBG("conn %p handle %u", conn, ev->conn_handle);

	bt_conn_unref(conn);
}
