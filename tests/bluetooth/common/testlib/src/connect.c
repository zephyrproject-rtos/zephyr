/* Copyright (c) 2023-2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <testlib/conn.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

LOG_MODULE_REGISTER(bt_testlib_connect, LOG_LEVEL_INF);

struct bt_testlib_connect_closure {
	uint8_t conn_cb_connected_err;
	struct bt_conn **connp;
	struct k_mutex lock;
	struct k_condvar conn_cb_connected_match;
};

/* Context pool (with capacity of one). */
static K_SEM_DEFINE(g_ctx_free, 1, 1);
static K_MUTEX_DEFINE(g_ctx_lock);
static struct bt_testlib_connect_closure *g_ctx;

static void on_conn_cb_connected(struct bt_conn *conn, uint8_t conn_err)
{
	/* Loop over each (allocated) item in pool. */

	k_mutex_lock(&g_ctx_lock, K_FOREVER);

	if (g_ctx && conn == *g_ctx->connp) {
		g_ctx->conn_cb_connected_err = conn_err;
		k_condvar_signal(&g_ctx->conn_cb_connected_match);
	}

	k_mutex_unlock(&g_ctx_lock);
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = on_conn_cb_connected,
};

int bt_testlib_connect(const bt_addr_le_t *peer, struct bt_conn **connp)
{
	int err;
	struct bt_testlib_connect_closure ctx = {
		.connp = connp,
	};
	uint8_t conn_index;

	__ASSERT_NO_MSG(connp);
	__ASSERT_NO_MSG(*connp == NULL);

	k_condvar_init(&ctx.conn_cb_connected_match);

	/* If multiple threads call into this funciton, they will wait
	 * for their turn here. The Zephyr host does not support
	 * concurrent connection creation.
	 */
	k_sem_take(&g_ctx_free, K_FOREVER);
	k_mutex_lock(&g_ctx_lock, K_FOREVER);
	g_ctx = &ctx;

	err = bt_conn_le_create(peer, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, connp);

	if (!err) {
		conn_index = bt_conn_index(*connp);
		LOG_INF("bt_conn_le_create ok conn %u", conn_index);

		k_condvar_wait(&ctx.conn_cb_connected_match, &g_ctx_lock, K_FOREVER);
	}

	g_ctx = NULL;
	k_mutex_unlock(&g_ctx_lock);
	k_sem_give(&g_ctx_free);

	/* Merge the error codes. The errors from `bt_conn_le_create`
	 * are negative, leaving the positive space for the HCI errors
	 * from `conn_cb_connected`.
	 */
	__ASSERT_NO_MSG(err <= 0);
	__ASSERT_NO_MSG(0 <= ctx.conn_cb_connected_err);
	__ASSERT_NO_MSG(!err || !ctx.conn_cb_connected_err);
	err = err + ctx.conn_cb_connected_err;

	/* This is just logging. */
	switch (err) {
	case -ENOMEM:
		LOG_INF("bt_conn_le_create -ENOMEM: No free connection objects available.");
		break;
	case 0:
		LOG_INF("conn %u: connected", conn_index);
		break;
	case BT_HCI_ERR_UNKNOWN_CONN_ID:
		LOG_INF("conn %u: timed out", conn_index);
		break;
	default:
		if (err < 0) {
			LOG_ERR("bt_conn_le_create err %d", err);
		} else {
			LOG_ERR("conn %u: BT_HCI_ERR_ 0x%02x", conn_index, err);
		}
	}

	/* Note: `connp` is never unrefed in this funciton, even in case
	 * of errors. This is as documented.
	 */

	return err;
}
