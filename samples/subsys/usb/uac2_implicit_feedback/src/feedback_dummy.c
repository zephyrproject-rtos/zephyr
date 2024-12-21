/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "feedback.h"

#warning "No target specific feedback code, overruns/underruns will occur"

struct feedback_ctx *feedback_init(void)
{
	return NULL;
}

void feedback_process(struct feedback_ctx *ctx)
{
	ARG_UNUSED(ctx);
}

void feedback_reset_ctx(struct feedback_ctx *ctx)
{
	ARG_UNUSED(ctx);
}

void feedback_start(struct feedback_ctx *ctx, int i2s_blocks_queued)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(i2s_blocks_queued);
}

int feedback_samples_offset(struct feedback_ctx *ctx)
{
	ARG_UNUSED(ctx);

	/* Always send nominal number of samples */
	return 0;
}
