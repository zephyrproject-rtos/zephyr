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

struct bt_testlib_adv_ctx {
	struct bt_conn **result;
	struct k_condvar done;
};

/* Context pool (with capacity of one). */
static K_SEM_DEFINE(g_ctx_free, 1, 1);
static K_MUTEX_DEFINE(g_ctx_lock);
static struct bt_testlib_adv_ctx *g_ctx;

static void connected_cb(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_connected_info *info)
{
	k_mutex_lock(&g_ctx_lock, K_FOREVER);

	if (g_ctx->result) {
		*g_ctx->result = bt_conn_ref(info->conn);
	}
	k_condvar_signal(&g_ctx->done);

	k_mutex_unlock(&g_ctx_lock);
}

int bt_testlib_adv_conn(struct bt_conn **conn, int id, const char *name)
{
	int api_err;
	struct bt_le_ext_adv *adv = NULL;
	struct bt_le_adv_param param = {};
	struct bt_testlib_adv_ctx ctx = {
		.result = conn,
	};
	static const struct bt_le_ext_adv_cb cb = {
		.connected = connected_cb,
	};

	param.id = id;
	param.interval_min = BT_GAP_ADV_FAST_INT_MIN_1;
	param.interval_max = BT_GAP_ADV_FAST_INT_MAX_1;
	param.options |= BT_LE_ADV_OPT_CONNECTABLE;

	k_condvar_init(&ctx.done);

	k_sem_take(&g_ctx_free, K_FOREVER);
	k_mutex_lock(&g_ctx_lock, K_FOREVER);
	g_ctx = &ctx;

	api_err = bt_le_ext_adv_create(&param, &cb, &adv);
	if (!api_err && name != NULL) {
		struct bt_data ad;

		ad.type = BT_DATA_NAME_COMPLETE;
		ad.data_len = strlen(name);
		ad.data = (const uint8_t *)name;

		api_err = bt_le_ext_adv_set_data(adv, &ad, 1, NULL, 0);
	}

	if (!api_err) {
		api_err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	}

	if (!api_err) {
		k_condvar_wait(&ctx.done, &g_ctx_lock, K_FOREVER);
	}

	/* Delete adv before giving semaphore so that it's potentially available
	 * for the next taker of the semaphore.
	 */
	if (adv) {
		bt_le_ext_adv_delete(adv);
	}

	g_ctx = NULL;
	k_mutex_unlock(&g_ctx_lock);
	k_sem_give(&g_ctx_free);

	return api_err;
}
