/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/graphic.h>
#include <zephyr/logging/log.h>
#include <zephyr/graphic/graphic.h>

LOG_MODULE_REGISTER(graphic_operation, CONFIG_GRAPHIC_LOG_LEVEL);

static int graphic_operation_area_check(struct graphic_area *area)
{
	unsigned int bpp;

	if (area == NULL) {
		return 0;
	}

	bpp = DISPLAY_BITS_PER_PIXEL(area->pixelformat);
	if (bpp == 0 || area->width == 0 || area->height == 0 ||
	    area->stride == 0 || area->addr == NULL) {
		return -EINVAL;
	}

	if (area->width > area->stride) {
		return -EINVAL;
	}

	return 0;
}

static int graphic_operation_filled_area_check(struct graphic_filled_area *area)
{
	if (area == NULL) {
		return 0;
	}

	if (area->width == 0 || area->height == 0) {
		return -EINVAL;
	}

	return 0;
}

static int graphic_operation_output_area_check(struct graphic_output_area *area)
{
	unsigned int bpp;

	if (area == NULL) {
		return 0;
	}

	bpp = DISPLAY_BITS_PER_PIXEL(area->pixelformat);

	if (bpp == 0 || area->stride == 0 || area->addr == 0) {
		return -EINVAL;
	}

	return 0;
}

static int graphic_get_free_operation_slot(struct graphic_context *ctx)
{
	for (int i = 0; i < ctx->pool_size; i++) {
		if (ctx->pool[i].status == GRAPHIC_OP_UNALLOCATED) {
			return i;
		}
	}

	return -ENOMEM;
}

int graphic_alloc_memcpy_conv_operation(const struct device *dev,
					struct graphic_output_area *output,
					struct graphic_area *input)
{
	struct graphic_operation *op;
	struct graphic_context *ctx;
	int op_id, ret;

	if (dev == NULL || input == NULL || output == NULL) {
		return -EINVAL;
	}

	ret = graphic_operation_area_check(input);
	if (ret < 0) {
		return ret;
	}

	ret = graphic_operation_output_area_check(output);
	if (ret < 0) {
		return ret;
	}

	ctx = dev->data;

	/* TODO - add a hook to go ask to the device if this is possible or not */

	k_mutex_lock(&ctx->lock, K_FOREVER);

	op_id = graphic_get_free_operation_slot(ctx);
	if (op_id < 0) {
		goto out;
	}

	op = &ctx->pool[op_id];
	op->status = GRAPHIC_OP_ALLOCATED;
	op->type = GRAPHIC_OP_MEMCPY_CONV;
	op->output = *output;
	op->u.input = *input;

out:
	k_mutex_unlock(&ctx->lock);

	return op_id;
}

int graphic_alloc_fill_operation(const struct device *dev, struct graphic_output_area *output,
				 struct graphic_filled_area *fill)
{
	struct graphic_operation *op;
	struct graphic_context *ctx;
	int op_id, ret = 0;

	if (dev == NULL || output == NULL || fill == NULL) {
		return -EINVAL;
	}

	ret = graphic_operation_output_area_check(output);
	if (ret < 0) {
		return ret;
	}

	ret = graphic_operation_filled_area_check(fill);
	if (ret < 0) {
		return ret;
	}

	ctx = dev->data;

	/* TODO - add a hook to go ask to the device if this is possible or not */

	k_mutex_lock(&ctx->lock, K_FOREVER);

	op_id = graphic_get_free_operation_slot(ctx);
	if (op_id < 0) {
		goto out;
	}

	op = &ctx->pool[op_id];
	op->status = GRAPHIC_OP_ALLOCATED;
	op->type = GRAPHIC_OP_FILL;
	op->output = *output;
	op->u.fill = *fill;

out:
	k_mutex_unlock(&ctx->lock);

	return op_id;
}

int graphic_free_operation(const struct device *dev, int op_id)
{
	struct graphic_context *ctx;
	int ret = 0;

	if (dev == NULL || op_id < 0) {
		return -EINVAL;
	}

	ctx = dev->data;

	k_mutex_lock(&ctx->lock, K_FOREVER);

	if (op_id >= ctx->pool_size || (ctx->pool[op_id].status != GRAPHIC_OP_ALLOCATED &&
					ctx->pool[op_id].status != GRAPHIC_OP_CANCELED &&
					ctx->pool[op_id].status != GRAPHIC_OP_COMPLETED &&
					ctx->pool[op_id].status != GRAPHIC_OP_ERROR)) {
		ret = -EINVAL;
		goto out;
	}

	ctx->pool[op_id].status = GRAPHIC_OP_UNALLOCATED;

out:
	k_mutex_unlock(&ctx->lock);

	return ret;
}

int graphic_submit_operation(const struct device *dev, int op_id)
{
	struct graphic_context *ctx;
	int ret = 0;

	if (dev == NULL || op_id < 0) {
		return -EINVAL;
	}

	ctx = dev->data;

	k_mutex_lock(&ctx->lock, K_FOREVER);

	if (op_id >= ctx->pool_size || (ctx->pool[op_id].status != GRAPHIC_OP_ALLOCATED &&
					ctx->pool[op_id].status != GRAPHIC_OP_CANCELED &&
					ctx->pool[op_id].status != GRAPHIC_OP_COMPLETED &&
					ctx->pool[op_id].status != GRAPHIC_OP_ERROR)) {
		ret = -EINVAL;
		goto out;
	}

	k_sem_init(&ctx->pool[op_id].sem, 0, 1);

	ctx->pool[op_id].status = GRAPHIC_OP_SUBMITTED;

	if (!ctx->running) {
		ctx->running = true;
		ctx->ongoing_op = &ctx->pool[op_id];
		ret = graphic_driver_process_operation(dev, &ctx->pool[op_id]);
		if (ret < 0) {
			LOG_ERR("Failed to start graphic process operation");
			ctx->pool[op_id].status = GRAPHIC_OP_ALLOCATED;
			ctx->running = false;
			goto out;
		}
	} else {
		k_fifo_put(&ctx->submitted, &ctx->pool[op_id]);
	}

out:
	k_mutex_unlock(&ctx->lock);

	return ret;
}

int graphic_sync_operation(const struct device *dev, int op_id, k_timeout_t timeout)
{
	struct graphic_context *ctx;

	if (dev == NULL || op_id < 0) {
		return -EINVAL;
	}

	ctx = dev->data;

	if (op_id >= ctx->pool_size || (ctx->pool[op_id].status != GRAPHIC_OP_SUBMITTED &&
					ctx->pool[op_id].status != GRAPHIC_OP_COMPLETED)) {
		return -EINVAL;
	}

	return k_sem_take(&ctx->pool[op_id].sem, timeout);
}

int graphic_get_operation_status(const struct device *dev, int op_id,
				 enum graphic_operation_status *status)
{
	struct graphic_context *ctx;
	int ret = 0;

	if (dev == NULL || status == NULL || op_id < 0) {
		return -EINVAL;
	}

	ctx = dev->data;

	k_mutex_lock(&ctx->lock, K_FOREVER);

	if (op_id >= ctx->pool_size) {
		ret = -EINVAL;
		goto out;
	}

	*status = ctx->pool[op_id].status;

out:
	k_mutex_unlock(&ctx->lock);

	return ret;
}
