/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <testlib/scan.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(bt_testlib_scan, LOG_LEVEL_INF);

struct bt_scan_find_name_closure {
	const char *wanted_name;
	bt_addr_le_t *result;
	struct k_condvar done;
};

/* Context pool (with capacity of one). */
static K_SEM_DEFINE(g_ctx_free, 1, 1);
static K_MUTEX_DEFINE(g_ctx_lock);
static struct bt_scan_find_name_closure *g_ctx;

static bool bt_scan_find_name_cb_data_cb(struct bt_data *data, void *user_data)
{
	const char **wanted = user_data;

	if (data->type == BT_DATA_NAME_COMPLETE) {
		if (data->data_len == strlen(*wanted) &&
		    !memcmp(*wanted, data->data, data->data_len)) {
			*wanted = NULL;
			/* Stop bt_data_parse. */
			return false;
		}
	}

	/* Continue with next ad data. */
	return true;
}

static void bt_scan_find_name_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
				 struct net_buf_simple *buf)
{
	const char *wanted;

	k_mutex_lock(&g_ctx_lock, K_FOREVER);

	__ASSERT_NO_MSG(g_ctx);
	__ASSERT_NO_MSG(g_ctx->wanted_name);

	wanted = g_ctx->wanted_name;

	bt_data_parse(buf, bt_scan_find_name_cb_data_cb, &wanted);

	if (!wanted) {
		(void)bt_le_scan_stop();
		*g_ctx->result = *addr;
		k_condvar_signal(&g_ctx->done);
	}

	k_mutex_unlock(&g_ctx_lock);
}

int bt_testlib_scan_find_name(bt_addr_le_t *result, const char *name)
{
	int api_err;
	struct bt_scan_find_name_closure ctx = {
		.wanted_name = name,
		.result = result,
	};

	k_condvar_init(&ctx.done);

	k_sem_take(&g_ctx_free, K_FOREVER);
	k_mutex_lock(&g_ctx_lock, K_FOREVER);
	g_ctx = &ctx;

	api_err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, bt_scan_find_name_cb);
	if (!api_err) {
		k_condvar_wait(&ctx.done, &g_ctx_lock, K_FOREVER);
	}

	g_ctx = NULL;
	k_mutex_unlock(&g_ctx_lock);
	k_sem_give(&g_ctx_free);

	if (!api_err) {
		char str[BT_ADDR_LE_STR_LEN];
		(void)bt_addr_le_to_str(result, str, ARRAY_SIZE(str));
		LOG_INF("Scan match: %s", str);
	} else {
		LOG_ERR("Scan error: %d", api_err);
	}

	return api_err;
}
