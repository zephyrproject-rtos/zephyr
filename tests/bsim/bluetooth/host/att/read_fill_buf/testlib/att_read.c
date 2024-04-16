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

#include "att_read.h"

struct bt_testlib_att_read_closure {
	uint8_t att_err;
	struct bt_conn *conn;
	struct bt_gatt_read_params params;
	uint16_t *result_size;
	uint16_t *result_handle;
	struct net_buf_simple *result_data;
	struct k_mutex lock;
	struct k_condvar done;
};

static inline uint8_t att_read_cb(struct bt_conn *conn, uint8_t att_err,
				  struct bt_gatt_read_params *params, const void *read_data,
				  uint16_t read_len)
{
	struct bt_testlib_att_read_closure *ctx =
		CONTAINER_OF(params, struct bt_testlib_att_read_closure, params);

	k_mutex_lock(&ctx->lock, K_FOREVER);

	ctx->att_err = att_err;

	if (!att_err && ctx->result_handle) {
		*ctx->result_handle = params->by_uuid.start_handle;
	}

	if (!att_err && ctx->result_size) {
		*ctx->result_size = read_len;
	}

	if (!att_err && ctx->result_data) {
		uint16_t result_data_size =
			MIN(read_len, net_buf_simple_tailroom(ctx->result_data));
		net_buf_simple_add_mem(ctx->result_data, read_data, result_data_size);
	}

	k_condvar_signal(&ctx->done);
	k_mutex_unlock(&ctx->lock);
	return BT_GATT_ITER_STOP;
}

static inline int bt_testlib_sync_bt_gatt_read(struct bt_testlib_att_read_closure *ctx)
{
	int api_err;

	ctx->params.func = att_read_cb;

	k_mutex_init(&ctx->lock);
	k_condvar_init(&ctx->done);

	k_mutex_lock(&ctx->lock, K_FOREVER);

	api_err = bt_gatt_read(ctx->conn, &ctx->params);

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

int bt_testlib_att_read_by_type_sync(struct net_buf_simple *result_data, uint16_t *result_size,
				     uint16_t *result_handle, struct bt_conn *conn,
				     enum bt_att_chan_opt bearer, const struct bt_uuid *type,
				     uint16_t start_handle, uint16_t end_handle)
{
	struct bt_testlib_att_read_closure ctx = {
		.result_handle = result_handle,
		.result_size = result_size,
		.conn = conn,
		.result_data = result_data,
		.params = {.by_uuid = {.uuid = type,
				       .start_handle = start_handle,
				       .end_handle = end_handle},
			   IF_ENABLED(CONFIG_BT_EATT, (.chan_opt = bearer))},
	};

	if (bearer == BT_ATT_CHAN_OPT_ENHANCED_ONLY) {
		__ASSERT(IS_ENABLED(CONFIG_BT_EATT), "EATT not complied in");
	}

	return bt_testlib_sync_bt_gatt_read(&ctx);
}

int bt_testlib_att_read_by_handle_sync(struct net_buf_simple *result_data, uint16_t *result_size,
				       struct bt_conn *conn, enum bt_att_chan_opt bearer,
				       uint16_t handle, uint16_t offset)
{
	struct bt_testlib_att_read_closure ctx = {
		.result_size = result_size,
		.conn = conn,
		.result_data = result_data,
		.params = {.handle_count = 1,
			   .single = {.handle = handle, .offset = offset},
			   IF_ENABLED(CONFIG_BT_EATT, (.chan_opt = bearer))},
	};

	if (bearer == BT_ATT_CHAN_OPT_ENHANCED_ONLY) {
		__ASSERT(IS_ENABLED(CONFIG_BT_EATT), "EATT not complied in");
	}

	return bt_testlib_sync_bt_gatt_read(&ctx);
}
