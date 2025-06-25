/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/usb/usbd.h>
#include "feedback.h"

#warning "No target specific feedback code, overruns/underruns will occur"

#define FEEDBACK_FS_K		10
#define FEEDBACK_FS_SHIFT	4
#define FEEDBACK_HS_K		13
#define FEEDBACK_HS_SHIFT	3

static struct feedback_ctx {
	bool high_speed;
} fb_ctx;

struct feedback_ctx *feedback_init(void)
{
	return &fb_ctx;
}

void feedback_process(struct feedback_ctx *ctx)
{
	ARG_UNUSED(ctx);
}

void feedback_reset_ctx(struct feedback_ctx *ctx, bool microframes)
{
	ctx->high_speed = microframes;
}

void feedback_start(struct feedback_ctx *ctx, int i2s_blocks_queued,
		    bool microframes)
{
	ARG_UNUSED(i2s_blocks_queued);

	ctx->high_speed = microframes;
}

uint32_t feedback_value(struct feedback_ctx *ctx)
{
	/* Always request nominal number of samples */
	if (USBD_SUPPORTS_HIGH_SPEED && ctx->high_speed) {
		return (SAMPLE_RATE / 8000) << (FEEDBACK_HS_K + FEEDBACK_HS_SHIFT);
	}

	return (SAMPLE_RATE / 1000) << (FEEDBACK_FS_K + FEEDBACK_FS_SHIFT);
}
