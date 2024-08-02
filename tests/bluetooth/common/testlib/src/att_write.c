/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_testlib_att_write, LOG_LEVEL_DBG);

struct bt_testlib_att_write_closure {
	uint8_t att_err;
	struct bt_gatt_write_params params;
	struct k_mutex lock;
	struct k_condvar done;
};

static void att_write_cb(struct bt_conn *conn, uint8_t att_err, struct bt_gatt_write_params *params)
{
	struct bt_testlib_att_write_closure *ctx =
		CONTAINER_OF(params, struct bt_testlib_att_write_closure, params);

	k_mutex_lock(&ctx->lock, K_FOREVER);

	ctx->att_err = att_err;

	k_condvar_signal(&ctx->done);
	k_mutex_unlock(&ctx->lock);
}

int bt_testlib_att_write(struct bt_conn *conn, enum bt_att_chan_opt bearer, uint16_t handle,
			 uint8_t *data, uint16_t size)
{
	int api_err;
	struct bt_testlib_att_write_closure ctx_val = {.params = {
							       .handle = handle,
							       .offset = 0,
							       .func = att_write_cb,
							       .data = data,
							       .length = size,
						       }};
	struct bt_testlib_att_write_closure *const ctx = &ctx_val;

	k_mutex_init(&ctx->lock);
	k_condvar_init(&ctx->done);

	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(
		IN_RANGE(handle, BT_ATT_FIRST_ATTRIBUTE_HANDLE, BT_ATT_LAST_ATTRIBUTE_HANDLE));

	k_mutex_lock(&ctx->lock, K_FOREVER);

	api_err = bt_gatt_write(conn, &ctx->params);

	if (!api_err) {
		k_condvar_wait(&ctx->done, &ctx->lock, K_FOREVER);
	}

	k_mutex_unlock(&ctx->lock);

	if (api_err) {
		__ASSERT_NO_MSG(api_err < 0);
		return api_err;
	}
	__ASSERT_NO_MSG(ctx->att_err >= 0);
	return ctx->att_err;
}
