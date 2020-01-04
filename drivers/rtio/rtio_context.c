/*
 * Copyright (c) 2019 Tom Burdick <tom.burdick@electromatic.us>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <drivers/rtio.h>
#include "rtio_context.h"

#define LOG_DOMAIN "rtio_context"
#define LOG_LEVEL CONFIG_RTIO_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(rtio_context);

static void rtio_context_output_timeout(struct k_timer *timer)
{
	struct rtio_context *ctx =
		CONTAINER_OF(timer, struct rtio_context, output_timer);

	int res = k_sem_take(&ctx->sem, K_NO_WAIT);

	if (res == 0) {
		if (ctx->block != NULL &&
		    ctx->config.output_config.fifo != NULL) {
			k_fifo_put(ctx->config.output_config.fifo, ctx->block);
			ctx->block = NULL;
		}
		k_sem_give(&ctx->sem);
	}
}

void rtio_context_init(struct rtio_context *ctx)
{
	k_sem_init(&ctx->sem, 0, 1);
	rtio_output_config_init(&ctx->config.output_config);
	ctx->block = NULL;
	k_timer_init(&ctx->output_timer, rtio_context_output_timeout, NULL);
	k_sem_give(&ctx->sem);
}

int rtio_context_configure_begin(struct rtio_context *ctx)
{
	k_sem_take(&ctx->sem, K_FOREVER);
	if (ctx->block != NULL && ctx->config.output_config.fifo != NULL) {
		k_fifo_put(ctx->config.output_config.fifo, ctx->block);
		ctx->block = NULL;
		return 1;
	}
	return 0;
}

void rtio_context_configure_end(struct rtio_context *ctx,
				struct rtio_config *config)
{
	s32_t timeout;

	memcpy(&ctx->config, config, sizeof(struct rtio_config));

	/* Setup timer if needed */
	k_timer_stop(&ctx->output_timer);
	timeout = ctx->config.output_config.timeout;
	if (timeout != K_FOREVER && timeout != K_NO_WAIT) {
		k_timer_start(&ctx->output_timer, timeout, timeout);
	}

	k_sem_give(&ctx->sem);
}

int rtio_context_trigger_read_begin(struct rtio_context *ctx,
				    struct rtio_block **block,
				    s32_t timeout)
{
	int res = 0;

	res = k_sem_take(&ctx->sem, timeout);
	if (res != 0) {
		return -EBUSY;
	}

	if (ctx->block == NULL) {
		res = rtio_block_alloc(ctx->config.output_config.allocator,
				       &ctx->block,
				       ctx->config.output_config.byte_size,
				       timeout);
		if (res != 0) {
			goto error;
		}
	}

	*block = ctx->block;
	return res;

error:
	k_sem_give(&ctx->sem);
	return res;
}

/**
 * @brief Check if a block has met an output policy expectations
 *
 * @return 1 if the policy has been met, 0 otherwise
 */
static inline int rtio_context_output_check(struct rtio_output_config *cfg,
					    struct rtio_block *block)
{
	/*
	if (k_cycle_get_32() - block->begin_tstamp > cfg->timeout) {
		return true;
	}
	*/
	if (rtio_block_used(block) >= cfg->byte_size) {
		return 1;
	}
	return 0;
}

int rtio_context_trigger_read_end(struct rtio_context *ctx)
{
	struct rtio_output_config *out_cfg = &ctx->config.output_config;
	int res;

	if (rtio_context_output_check(out_cfg, ctx->block) == 1) {
		k_fifo_put(out_cfg->fifo, ctx->block);
		ctx->block = NULL;
		res = 1;
	} else {
		res = 0;
	}

	k_sem_give(&ctx->sem);
	return res;
}
