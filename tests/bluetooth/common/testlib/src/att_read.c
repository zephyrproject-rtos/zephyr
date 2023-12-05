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

#include <testlib/att_read.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_testlib_att_read, LOG_LEVEL_DBG);

struct bt_testlib_att_read_closure {
	uint8_t att_err;
	struct bt_conn *conn;
	struct bt_gatt_read_params params;
	uint16_t *result_size;
	uint16_t *result_handle;
	struct net_buf_simple *result_data;
	struct k_mutex lock;
	struct k_condvar done;
	uint16_t *att_mtu;
	bool long_read;
};

static bool bt_gatt_read_params_is_by_uuid(const struct bt_gatt_read_params *params)
{
	return params->handle_count == 0;
}

static uint8_t att_read_cb(struct bt_conn *conn, uint8_t att_err,
			   struct bt_gatt_read_params *params, const void *read_data,
			   uint16_t read_len)
{
	struct bt_testlib_att_read_closure *ctx =
		CONTAINER_OF(params, struct bt_testlib_att_read_closure, params);

	k_mutex_lock(&ctx->lock, K_FOREVER);

	ctx->att_err = att_err;

	if (!att_err && ctx->result_handle) {
		__ASSERT_NO_MSG(bt_gatt_read_params_is_by_uuid(params));
		*ctx->result_handle = params->by_uuid.start_handle;
	}

	if (!att_err && ctx->result_size) {
		LOG_DBG("Adding %u bytes to result", read_len);
		*ctx->result_size += read_len;
		if (*ctx->result_size > BT_ATT_MAX_ATTRIBUTE_LEN) {
			LOG_ERR("result_size > 512");
		}
	}

	if (read_data && ctx->result_data) {
		uint16_t result_data_size =
			MIN(read_len, net_buf_simple_tailroom(ctx->result_data));

		net_buf_simple_add_mem(ctx->result_data, read_data, result_data_size);
	}

	if (!att_err && ctx->att_mtu) {
		*ctx->att_mtu = params->_att_mtu;
	}

	if (ctx->long_read && read_data) {
		/* Don't signal `&ctx->done` */
		k_mutex_unlock(&ctx->lock);
		return BT_GATT_ITER_CONTINUE;
	}

	k_condvar_signal(&ctx->done);
	k_mutex_unlock(&ctx->lock);
	return BT_GATT_ITER_STOP;
}

static int bt_testlib_sync_bt_gatt_read(struct bt_testlib_att_read_closure *ctx)
{
	int api_err;

	/* `result_size` is initialized here so that it can be plussed on in
	 * the callback. The result of a long read comes in multiple
	 * callbacks and must be added up.
	 */
	if (ctx->result_size) {
		*ctx->result_size = 0;
	}

	/* `att_read_cb` is smart and does the right thing based on `ctx`. */
	ctx->params.func = att_read_cb;

	/* Setup synchronization between the cb and the current function. */
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
				     uint16_t *result_handle, uint16_t *result_att_mtu,
				     struct bt_conn *conn, enum bt_att_chan_opt bearer,
				     const struct bt_uuid *type, uint16_t start_handle,
				     uint16_t end_handle)
{
	struct bt_testlib_att_read_closure ctx = {.result_handle = result_handle,
						  .result_size = result_size,
						  .conn = conn,
						  .result_data = result_data,
						  .att_mtu = result_att_mtu,
						  .params = {
							  .by_uuid = {.uuid = type,
								      .start_handle = start_handle,
								      .end_handle = end_handle},
						  }};

	IF_ENABLED(CONFIG_BT_EATT, ({ ctx.params.chan_opt = bearer; }))

	if (bearer == BT_ATT_CHAN_OPT_ENHANCED_ONLY) {
		__ASSERT(IS_ENABLED(CONFIG_BT_EATT), "EATT not complied in");
	}

	return bt_testlib_sync_bt_gatt_read(&ctx);
}

int bt_testlib_att_read_by_handle_sync(struct net_buf_simple *result_data, uint16_t *result_size,
				       uint16_t *result_att_mtu, struct bt_conn *conn,
				       enum bt_att_chan_opt bearer, uint16_t handle,
				       uint16_t offset)
{
	struct bt_testlib_att_read_closure ctx = {};

	ctx.att_mtu = result_att_mtu;
	ctx.conn = conn;
	ctx.params.handle_count = 1;
	ctx.params.single.handle = handle;
	ctx.params.single.offset = offset;
	ctx.result_data = result_data;
	ctx.result_size = result_size;
	IF_ENABLED(CONFIG_BT_EATT, (ctx.params.chan_opt = bearer));

	if (bearer == BT_ATT_CHAN_OPT_ENHANCED_ONLY) {
		__ASSERT(IS_ENABLED(CONFIG_BT_EATT), "EATT not complied in");
	}

	return bt_testlib_sync_bt_gatt_read(&ctx);
}

int bt_testlib_gatt_long_read(struct net_buf_simple *result_data, uint16_t *result_size,
			      uint16_t *result_att_mtu, struct bt_conn *conn,
			      enum bt_att_chan_opt bearer, uint16_t handle, uint16_t offset)
{
	int err;
	uint16_t _result_data_size = 0;
	struct bt_testlib_att_read_closure ctx = {};

	ctx.att_mtu = result_att_mtu;
	ctx.conn = conn;
	ctx.long_read = true, ctx.params.handle_count = 1;
	ctx.params.single.handle = handle;
	ctx.params.single.offset = offset;
	ctx.result_data = result_data;
	ctx.result_size = &_result_data_size;
	IF_ENABLED(CONFIG_BT_EATT, (ctx.params.chan_opt = bearer));

	if (bearer == BT_ATT_CHAN_OPT_ENHANCED_ONLY) {
		__ASSERT(IS_ENABLED(CONFIG_BT_EATT), "EATT not complied in");
	}

	err = bt_testlib_sync_bt_gatt_read(&ctx);

	if (result_size) {
		*result_size = _result_data_size;
	}

	return err;
}

struct bt_testlib_gatt_discover_service_closure {
	struct bt_gatt_discover_params params;
	uint8_t att_err;
	uint16_t *const result_handle;
	uint16_t *const result_end_handle;
	struct k_mutex lock;
	struct k_condvar done;
};

static uint8_t gatt_discover_service_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					struct bt_gatt_discover_params *params)
{
	struct bt_testlib_gatt_discover_service_closure *ctx =
		CONTAINER_OF(params, struct bt_testlib_gatt_discover_service_closure, params);

	k_mutex_lock(&ctx->lock, K_FOREVER);

	ctx->att_err = attr ? BT_ATT_ERR_SUCCESS : BT_ATT_ERR_ATTRIBUTE_NOT_FOUND;

	if (!ctx->att_err) {
		if (ctx->result_handle) {
			*ctx->result_handle = attr->handle;
		}

		if (ctx->result_end_handle) {
			*ctx->result_end_handle = 0;
			/* Output 'group end handle'. */
			if (params->type == BT_GATT_DISCOVER_PRIMARY ||
			    params->type == BT_GATT_DISCOVER_SECONDARY) {
				*ctx->result_end_handle =
					((struct bt_gatt_service_val *)attr->user_data)->end_handle;
			}
		}
	}

	k_condvar_signal(&ctx->done);
	k_mutex_unlock(&ctx->lock);
	return BT_GATT_ITER_STOP;
}

/** AKA Service discovery by UUID.
 */
int bt_testlib_gatt_discover_primary(uint16_t *result_handle, uint16_t *result_end_handle,
				     struct bt_conn *conn, const struct bt_uuid *uuid,
				     uint16_t start_handle, uint16_t end_handle)
{
	int api_err;

	struct bt_testlib_gatt_discover_service_closure ctx_val = {
		.result_handle = result_handle,
		.result_end_handle = result_end_handle,
		.params = {
			.type = BT_GATT_DISCOVER_PRIMARY,
			.start_handle = start_handle,
			.end_handle = end_handle,
			.func = gatt_discover_service_cb,
			.uuid = uuid,
		}};
	struct bt_testlib_gatt_discover_service_closure *const ctx = &ctx_val;

	k_mutex_init(&ctx->lock);
	k_condvar_init(&ctx->done);

	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(IN_RANGE(start_handle, BT_ATT_FIRST_ATTRIBUTE_HANDLE,
				 BT_ATT_LAST_ATTRIBUTE_HANDLE));
	__ASSERT_NO_MSG(
		IN_RANGE(end_handle, BT_ATT_FIRST_ATTRIBUTE_HANDLE, BT_ATT_LAST_ATTRIBUTE_HANDLE));

	k_mutex_lock(&ctx->lock, K_FOREVER);

	api_err = bt_gatt_discover(conn, &ctx->params);

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

struct bt_testlib_gatt_discover_char_closure {
	struct bt_gatt_discover_params params;
	uint8_t att_err;
	uint16_t *const result_def_handle;
	uint16_t *const result_value_handle;
	uint16_t *const result_end_handle;
	uint16_t svc_end_handle;
	struct k_mutex lock;
	struct k_condvar done;
};

static uint8_t gatt_discover_char_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	struct bt_testlib_gatt_discover_char_closure *ctx =
		CONTAINER_OF(params, struct bt_testlib_gatt_discover_char_closure, params);
	bool read_more = false;

	k_mutex_lock(&ctx->lock, K_FOREVER);

	if (ctx->att_err == BT_ATT_ERR_ATTRIBUTE_NOT_FOUND) {
		/* The start of the charachteristic was not found yet.
		 * This is the start of the characteristic.
		 */
		if (attr) {
			ctx->att_err = BT_ATT_ERR_SUCCESS;
			if (ctx->result_def_handle) {
				*ctx->result_def_handle = attr->handle;
			}
			if (ctx->result_value_handle) {
				*ctx->result_value_handle =
					((struct bt_gatt_chrc *)attr->user_data)->value_handle;
			}
			if (ctx->result_end_handle) {
				read_more = true;
			}
		}
	} else {
		/* This is the end of the characteristic.
		 */
		if (attr) {
			__ASSERT_NO_MSG(ctx->result_end_handle);
			*ctx->result_end_handle = (attr->handle - 1);
		}
	};

	if (!read_more) {
		k_condvar_signal(&ctx->done);
	}
	k_mutex_unlock(&ctx->lock);
	return read_more ? BT_GATT_ITER_CONTINUE : BT_GATT_ITER_STOP;
}

int bt_testlib_gatt_discover_characteristic(uint16_t *const result_value_handle,
					    uint16_t *const result_end_handle,
					    uint16_t *const result_def_handle, struct bt_conn *conn,
					    const struct bt_uuid *uuid, uint16_t start_handle,
					    uint16_t svc_end_handle)
{
	int api_err;

	if (result_end_handle) {
		/* If there is no second result, the end_handle is the svc_end. */
		*result_end_handle = svc_end_handle;
	}

	struct bt_testlib_gatt_discover_char_closure ctx_val = {
		.att_err = BT_ATT_ERR_ATTRIBUTE_NOT_FOUND,
		.result_value_handle = result_value_handle,
		.result_def_handle = result_def_handle,
		.result_end_handle = result_end_handle,
		.params = {
			.type = BT_GATT_DISCOVER_CHARACTERISTIC,
			.start_handle = start_handle,
			.end_handle = svc_end_handle,
			.func = gatt_discover_char_cb,
			.uuid = uuid,
		}};
	struct bt_testlib_gatt_discover_char_closure *const ctx = &ctx_val;

	k_mutex_init(&ctx->lock);
	k_condvar_init(&ctx->done);

	__ASSERT_NO_MSG(conn);
	__ASSERT_NO_MSG(IN_RANGE(start_handle, BT_ATT_FIRST_ATTRIBUTE_HANDLE,
				 BT_ATT_LAST_ATTRIBUTE_HANDLE));
	__ASSERT_NO_MSG(IN_RANGE(svc_end_handle, BT_ATT_FIRST_ATTRIBUTE_HANDLE,
				 BT_ATT_LAST_ATTRIBUTE_HANDLE));

	k_mutex_lock(&ctx->lock, K_FOREVER);

	api_err = bt_gatt_discover(conn, &ctx->params);

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
