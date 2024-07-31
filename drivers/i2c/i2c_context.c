/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "i2c_context.h"

#define CONTEXT_TRANSFER_TIMEOUT K_MSEC(CONFIG_I2C_CONTEXT_TRANSFER_TIMEOUT_MS)

#if CONFIG_I2C_CALLBACK
static void context_transfer_stop(struct i2c_context *ctx, int result)
{
	i2c_callback_t transfer_callback;
	void *transfer_callback_userdata;

	ctx->transfer_started = false;
	ctx->transfer_result = result;

	k_work_cancel_delayable(&ctx->transfer_timeout_dwork);

	ctx->deinit_transfer_handler(ctx);

	if (ctx->transfer_callback == NULL) {
		k_sem_give(&ctx->transfer_sync);
		return;
	}

	transfer_callback = ctx->transfer_callback;
	transfer_callback_userdata = ctx->transfer_callback_userdata;

	k_sem_give(&ctx->transfer_lock);

	if (transfer_callback != NULL) {
		transfer_callback(ctx->dev, result, transfer_callback_userdata);
	}
}

static void context_start_transfer_handler(struct k_work *work)
{
	struct i2c_context *ctx = CONTAINER_OF(work, struct i2c_context, start_transfer_work);

	ctx->transfer_started = true;

	ctx->start_transfer_handler(ctx);
}

static void context_continue_transfer_handler(struct k_work *work)
{
	struct i2c_context *ctx = CONTAINER_OF(work, struct i2c_context, continue_transfer_work);

	if (!ctx->transfer_started) {
		return;
	}

	ctx->post_transfer_handler(ctx);

	ctx->transfer_msg_idx++;

	if (ctx->transfer_msg_idx == ctx->transfer_num_msgs) {
		context_transfer_stop(ctx, 0);
		return;
	}

	ctx->start_transfer_handler(ctx);
}

static void context_cancel_transfer_handler(struct k_work *work)
{
	struct i2c_context *ctx = CONTAINER_OF(work, struct i2c_context, cancel_transfer_work);

	if (!ctx->transfer_started) {
		return;
	}

	context_transfer_stop(ctx, -EIO);
}

static void context_transfer_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct i2c_context *ctx = CONTAINER_OF(dwork, struct i2c_context, transfer_timeout_dwork);

	if (!ctx->transfer_started) {
		return;
	}

	context_transfer_stop(ctx, -ETIMEDOUT);
}
#endif

void i2c_context_init(struct i2c_context *ctx,
		      const struct device *dev,
		      i2c_context_init_transfer_handler init_transfer_handler,
		      i2c_context_start_transfer_handler start_transfer_handler,
		      i2c_context_post_transfer_handler post_transfer_handler,
		      i2c_context_deinit_transfer_handler deinit_transfer_handler)
{
	ctx->dev = dev;
	ctx->init_transfer_handler = init_transfer_handler;
	ctx->start_transfer_handler = start_transfer_handler;
	ctx->post_transfer_handler = post_transfer_handler;
	ctx->deinit_transfer_handler = deinit_transfer_handler;

	k_sem_init(&ctx->transfer_lock, 1, 1);
	k_sem_init(&ctx->transfer_sync, 0, 1);

#if CONFIG_I2C_CALLBACK
	k_work_init(&ctx->start_transfer_work, context_start_transfer_handler);
	k_work_init(&ctx->continue_transfer_work, context_continue_transfer_handler);
	k_work_init(&ctx->cancel_transfer_work, context_cancel_transfer_handler);
	k_work_init_delayable(&ctx->transfer_timeout_dwork, context_transfer_timeout_handler);
#endif
}

int i2c_context_start_transfer(struct i2c_context *ctx,
			       struct i2c_msg *msgs,
			       uint8_t num_msgs,
			       uint16_t addr)
{
	int ret;

	k_sem_take(&ctx->transfer_lock, K_FOREVER);

	ctx->transfer_msgs = msgs;
	ctx->transfer_num_msgs = num_msgs;
	ctx->transfer_addr = addr;
	ctx->transfer_msg_idx = 0;

	ret = ctx->init_transfer_handler(ctx);

	if (ret) {
		k_sem_give(&ctx->transfer_lock);
		return ret;
	}

#if CONFIG_I2C_CALLBACK
	k_sem_reset(&ctx->transfer_sync);

	k_work_submit(&ctx->start_transfer_work);
	k_work_schedule(&ctx->transfer_timeout_dwork, CONTEXT_TRANSFER_TIMEOUT);

	k_sem_take(&ctx->transfer_sync, K_FOREVER);
#else
	for (ctx->transfer_msg_idx = 0;
	     ctx->transfer_msg_idx < ctx->transfer_num_msgs;
	     ctx->transfer_msg_idx++) {
		k_sem_reset(&ctx->transfer_sync);

		ctx->start_transfer_handler(ctx);

		if (k_sem_take(&ctx->transfer_sync, CONTEXT_TRANSFER_TIMEOUT)) {
			ctx->transfer_result = -EIO;
			break;
		}

		if (ctx->transfer_result) {
			break;
		}

		ctx->post_transfer_handler(ctx);
	}

	ctx->deinit_transfer_handler(ctx);
#endif

	ret = ctx->transfer_result;
	k_sem_give(&ctx->transfer_lock);
	return ret;
}

#if CONFIG_I2C_CALLBACK
int i2c_context_start_transfer_cb(struct i2c_context *ctx,
				  struct i2c_msg *msgs,
				  uint8_t num_msgs,
				  uint16_t addr,
				  i2c_callback_t cb,
				  void *userdata)
{
	int ret;

	if (k_sem_take(&ctx->transfer_lock, K_NO_WAIT)) {
		return -EWOULDBLOCK;
	}

	ctx->transfer_msgs = msgs;
	ctx->transfer_num_msgs = num_msgs;
	ctx->transfer_addr = addr;
	ctx->transfer_msg_idx = 0;
	ctx->transfer_callback = cb;
	ctx->transfer_callback_userdata = userdata;

	ret = ctx->init_transfer_handler(ctx);

	if (ret) {
		k_sem_give(&ctx->transfer_lock);
		return ret;
	}

	k_work_submit(&ctx->start_transfer_work);
	k_work_schedule(&ctx->transfer_timeout_dwork, CONTEXT_TRANSFER_TIMEOUT);
	return ret;
}
#endif

void i2c_context_continue_transfer(struct i2c_context *ctx)
{
#if CONFIG_I2C_CALLBACK
	k_work_submit(&ctx->continue_transfer_work);
#else
	ctx->transfer_result = 0;
	k_sem_give(&ctx->transfer_sync);
#endif
}

void i2c_context_cancel_transfer(struct i2c_context *ctx)
{
#if CONFIG_I2C_CALLBACK
	k_work_submit(&ctx->cancel_transfer_work);
#else
	ctx->transfer_result = -EIO;
	k_sem_give(&ctx->transfer_sync);
#endif
}
