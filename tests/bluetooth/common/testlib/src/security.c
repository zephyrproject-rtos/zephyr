/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_testlib_security, LOG_LEVEL_INF);

struct testlib_security_ctx {
	enum bt_security_err result;
	struct bt_conn *conn;
	bt_security_t new_minimum;
	struct k_condvar done;
};

/* Context pool (with capacity of one). */
static K_SEM_DEFINE(g_ctx_free, 1, 1);
static K_MUTEX_DEFINE(g_ctx_lock);
static struct testlib_security_ctx *g_ctx;

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	LOG_INF("conn %u level %d err %d", bt_conn_index(conn), level, err);

	/* Mutex operations establish a happens-before relationship. This
	 * ensures variables have the expected values despite non-atomic
	 * accesses.
	 */
	k_mutex_lock(&g_ctx_lock, K_FOREVER);

	if (g_ctx && (g_ctx->conn == conn)) {
		g_ctx->result = err;
		/* Assumption: A security error means there will be further
		 * security changes for this connection.
		 */
		if (err || level >= g_ctx->new_minimum) {
			k_condvar_signal(&g_ctx->done);
		}
	}

	k_mutex_unlock(&g_ctx_lock);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.security_changed = security_changed,
};

int bt_testlib_secure(struct bt_conn *conn, bt_security_t new_minimum)
{
	int api_err = 0;
	struct testlib_security_ctx ctx = {
		.conn = conn,
		.new_minimum = new_minimum,
	};

	k_condvar_init(&ctx.done);

	/* The semaphore allocates `g_ctx` to this invocation of
	 * `bt_testlib_secure`, in case this function is called from multiple
	 * threads in parallel.
	 */
	k_sem_take(&g_ctx_free, K_FOREVER);
	/* The mutex synchronizes this function with `security_changed()`. */
	k_mutex_lock(&g_ctx_lock, K_FOREVER);

	/* Do the thing. */
	api_err = bt_conn_set_security(conn, new_minimum);

	/* Holding the mutex will pause any thread entering
	 * `security_changed_cb`, delaying it until `k_condvar_wait`. This
	 * ensures that the condition variable is signaled while this thread is
	 * in `k_condvar_wait`, even if the event happens before, e.g. between
	 * `bt_conn_get_security` and `k_condvar_wait`.
	 *
	 * If the security level is already satisfied, there is no point in
	 * waiting, and it would deadlock if security was already satisfied
	 * before the mutex was taken, `bt_conn_set_security` will result in no
	 * operation.
	 */
	if (!api_err && bt_conn_get_security(conn) < new_minimum) {
		/* Waiting on a condvar releases the mutex and waits for a
		 * signal on the condvar, atomically, without a gap between the
		 * release and wait. The mutex is locked again before returning.
		 */
		g_ctx = &ctx;
		k_condvar_wait(&ctx.done, &g_ctx_lock, K_FOREVER);
		g_ctx = NULL;
	}

	k_mutex_unlock(&g_ctx_lock);
	k_sem_give(&g_ctx_free);

	if (api_err) {
		__ASSERT_NO_MSG(api_err < 0);
		return api_err;
	}

	__ASSERT_NO_MSG(ctx.result >= 0);
	return ctx.result;
}
