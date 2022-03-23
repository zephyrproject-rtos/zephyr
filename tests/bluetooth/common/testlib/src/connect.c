/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <testlib/conn.h>

LOG_MODULE_REGISTER(bt_testlib_connect, LOG_LEVEL_INF);

struct bt_testlib_connect_closure {
	uint8_t conn_err;
	struct bt_conn **conn;
	struct k_mutex lock;
	struct k_condvar done;
};

/* Context pool (with capacity of one). */
static K_SEM_DEFINE(g_ctx_free, 1, 1);
static K_MUTEX_DEFINE(g_ctx_lock);
static struct bt_testlib_connect_closure *g_ctx;

static void connected_cb(struct bt_conn *conn, uint8_t conn_err)
{
	/* Loop over each (allocated) item in pool. */

	k_mutex_lock(&g_ctx_lock, K_FOREVER);

	if (g_ctx && conn == *g_ctx->conn) {
		g_ctx->conn_err = conn_err;
		k_condvar_signal(&g_ctx->done);
	}

	k_mutex_unlock(&g_ctx_lock);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected_cb,
};

int bt_testlib_connect(const bt_addr_le_t *peer, struct bt_conn **conn)
{
	int api_err;
	struct bt_testlib_connect_closure ctx = {
		.conn = conn,
	};

	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(*conn == NULL);

	k_condvar_init(&ctx.done);

	k_sem_take(&g_ctx_free, K_FOREVER);
	k_mutex_lock(&g_ctx_lock, K_FOREVER);
	g_ctx = &ctx;

	api_err = bt_conn_le_create(peer, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, conn);

	if (!api_err) {
		LOG_INF("Connecting.. conn %u", bt_conn_index(*conn));
		k_condvar_wait(&ctx.done, &g_ctx_lock, K_FOREVER);
		LOG_INF("Connect complete");
	}

	g_ctx = NULL;
	k_mutex_unlock(&g_ctx_lock);
	k_sem_give(&g_ctx_free);

	if (api_err) {
		LOG_ERR("bt_conn_le_create err %d", api_err);
		__ASSERT_NO_MSG(api_err < 0);
		return api_err;
	}

	if (ctx.conn_err) {
		LOG_ERR("Connect HCI err %d", ctx.conn_err);
		__ASSERT_NO_MSG(ctx.conn_err >= 0);
		return ctx.conn_err;
	}

	return 0;
}
