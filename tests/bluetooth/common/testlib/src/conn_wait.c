/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>

#include <testlib/conn.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_testlib_conn_wait, LOG_LEVEL_DBG);

static K_MUTEX_DEFINE(conn_wait_mutex);
static K_CONDVAR_DEFINE(something_changed);

static void on_change(struct bt_conn *conn, uint8_t err)
{
	k_mutex_lock(&conn_wait_mutex, K_FOREVER);
	k_condvar_broadcast(&something_changed);
	k_mutex_unlock(&conn_wait_mutex);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = on_change,
	.disconnected = on_change,
};

static enum bt_conn_state bt_conn_state(struct bt_conn *conn)
{
	int err;
	struct bt_conn_info info;

	__ASSERT(conn != NULL, "Invalid connection");
	err = bt_conn_get_info(conn, &info);
	__ASSERT(err == 0, "Failed to get connection info");

	return info.state;
}

int bt_testlib_wait_connected(struct bt_conn *conn)
{
	__ASSERT_NO_MSG(conn != NULL);
	k_mutex_lock(&conn_wait_mutex, K_FOREVER);
	while (bt_conn_state(conn) != BT_CONN_STATE_CONNECTED) {
		k_condvar_wait(&something_changed, &conn_wait_mutex, K_FOREVER);
	}
	k_mutex_unlock(&conn_wait_mutex);
	return 0;
}

int bt_testlib_wait_disconnected(struct bt_conn *conn)
{
	__ASSERT_NO_MSG(conn != NULL);
	k_mutex_lock(&conn_wait_mutex, K_FOREVER);
	while (bt_conn_state(conn) != BT_CONN_STATE_DISCONNECTED) {
		k_condvar_wait(&something_changed, &conn_wait_mutex, K_FOREVER);
	}
	k_mutex_unlock(&conn_wait_mutex);
	return 0;
}
