/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/logging/log.h>
#include "feedback.h"

#include <nrfx_dppi.h>
#include <nrfx_timer.h>
#include <hal/nrf_usbd.h>
#include <hal/nrf_i2s.h>
#include <helpers/nrfx_gppi.h>

LOG_MODULE_REGISTER(feedback, LOG_LEVEL_INF);

#define FEEDBACK_TIMER_INSTANCE_NUMBER 2
#define FEEDBACK_TIMER_USBD_SOF_CAPTURE 0
#define FEEDBACK_TIMER_I2S_FRAMESTART_CAPTURE 1

static const nrfx_timer_t feedback_timer_instance =
	NRFX_TIMER_INSTANCE(FEEDBACK_TIMER_INSTANCE_NUMBER);

/* While it might be possible to determine I2S FRAMESTART to USB SOF offset
 * entirely in software, the I2S API lacks appropriate timestamping. Therefore
 * this sample uses target-specific code to perform the measurements. Note that
 * the use of dedicated target-specific peripheral essentially eliminates
 * software scheduling jitter and it is likely that a pure software only
 * solution would require additional filtering in indirect offset measurements.
 *
 * Use timer clock (independent from both Audio clock and USB host SOF clock)
 * values directly to determine samples offset. This works fine because the
 * regulator cares only about error (SOF offset is both error and regulator
 * input) and achieves its goal by sending nominal + 1 or nominal - 1 samples.
 * SOF offset is around 0 when regulated and therefore the relative clock
 * frequency discrepancies are essentially negligible.
 */
#define CLKS_PER_SAMPLE	(16000000 / (SAMPLES_PER_SOF * 1000))

static struct feedback_ctx {
	int32_t rel_sof_offset;
	int32_t base_sof_offset;
} fb_ctx;

struct feedback_ctx *feedback_init(void)
{
	nrfx_err_t err;
	uint8_t usbd_sof_gppi_channel;
	uint8_t i2s_framestart_gppi_channel;
	const nrfx_timer_config_t cfg = {
		.frequency = NRFX_MHZ_TO_HZ(16UL),
		.mode = NRF_TIMER_MODE_TIMER,
		.bit_width = NRF_TIMER_BIT_WIDTH_32,
		.interrupt_priority = NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
		.p_context = NULL,
	};

	feedback_reset_ctx(&fb_ctx);

	err = nrfx_timer_init(&feedback_timer_instance, &cfg, NULL);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx timer init error - Return value: %d", err);
		return &fb_ctx;
	}

	/* Subscribe TIMER CAPTURE task to USBD SOF event */
	err = nrfx_gppi_channel_alloc(&usbd_sof_gppi_channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("gppi_channel_alloc failed with: %d\n", err);
		return &fb_ctx;
	}

	nrfx_gppi_channel_endpoints_setup(usbd_sof_gppi_channel,
		nrf_usbd_event_address_get(NRF_USBD, NRF_USBD_EVENT_SOF),
		nrfx_timer_capture_task_address_get(&feedback_timer_instance,
			FEEDBACK_TIMER_USBD_SOF_CAPTURE));
	nrfx_gppi_fork_endpoint_setup(usbd_sof_gppi_channel,
		nrfx_timer_task_address_get(&feedback_timer_instance,
			NRF_TIMER_TASK_CLEAR));

	nrfx_gppi_channels_enable(BIT(usbd_sof_gppi_channel));

	/* Subscribe TIMER CAPTURE task to I2S FRAMESTART event */
	err = nrfx_gppi_channel_alloc(&i2s_framestart_gppi_channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("gppi_channel_alloc failed with: %d\n", err);
		return &fb_ctx;
	}

	nrfx_gppi_channel_endpoints_setup(i2s_framestart_gppi_channel,
		nrf_i2s_event_address_get(NRF_I2S0, NRF_I2S_EVENT_FRAMESTART),
		nrfx_timer_capture_task_address_get(&feedback_timer_instance,
			FEEDBACK_TIMER_I2S_FRAMESTART_CAPTURE));

	nrfx_gppi_channels_enable(BIT(i2s_framestart_gppi_channel));

	/* Enable feedback timer */
	nrfx_timer_enable(&feedback_timer_instance);

	return &fb_ctx;
}

static void update_sof_offset(struct feedback_ctx *ctx, uint32_t sof_cc,
			      uint32_t framestart_cc)
{
	int sof_offset;

	/* /2 because we treat the middle as a turning point from being
	 * "too late" to "too early".
	 */
	if (framestart_cc > (SAMPLES_PER_SOF * CLKS_PER_SAMPLE)/2) {
		sof_offset = framestart_cc - SAMPLES_PER_SOF * CLKS_PER_SAMPLE;
	} else {
		sof_offset = framestart_cc;
	}

	/* The heuristic above is not enough when the offset gets too large.
	 * If the sign of the simple heuristic changes, check whether the offset
	 * crossed through the zero or the outer bound.
	 */
	if ((ctx->rel_sof_offset >= 0) != (sof_offset >= 0)) {
		uint32_t abs_diff;
		int32_t base_change;

		if (sof_offset >= 0) {
			abs_diff = sof_offset - ctx->rel_sof_offset;
			base_change = -(SAMPLES_PER_SOF * CLKS_PER_SAMPLE);
		} else {
			abs_diff = ctx->rel_sof_offset - sof_offset;
			base_change = SAMPLES_PER_SOF * CLKS_PER_SAMPLE;
		}

		/* Adjust base offset only if the change happened through the
		 * outer bound. The actual changes should be significantly lower
		 * than the threshold here.
		 */
		if (abs_diff > (SAMPLES_PER_SOF * CLKS_PER_SAMPLE)/2) {
			ctx->base_sof_offset += base_change;
		}
	}

	ctx->rel_sof_offset = sof_offset;
}

void feedback_process(struct feedback_ctx *ctx)
{
	uint32_t sof_cc;
	uint32_t framestart_cc;

	sof_cc = nrfx_timer_capture_get(&feedback_timer_instance,
		FEEDBACK_TIMER_USBD_SOF_CAPTURE);
	framestart_cc = nrfx_timer_capture_get(&feedback_timer_instance,
		FEEDBACK_TIMER_I2S_FRAMESTART_CAPTURE);

	update_sof_offset(ctx, sof_cc, framestart_cc);
}

void feedback_reset_ctx(struct feedback_ctx *ctx)
{
	ARG_UNUSED(ctx);
}

void feedback_start(struct feedback_ctx *ctx, int i2s_blocks_queued)
{
	/* I2S data was supposed to go out at SOF, but it is inevitably
	 * delayed due to triggering I2S start by software. Set relative
	 * SOF offset value in a way that ensures that values past "half
	 * frame" are treated as "too late" instead of "too early"
	 */
	ctx->rel_sof_offset = (SAMPLES_PER_SOF * CLKS_PER_SAMPLE) / 2;
	/* If there are more than 2 I2S TX blocks queued, use feedback regulator
	 * to correct the situation.
	 */
	ctx->base_sof_offset = (i2s_blocks_queued - 2) *
		(SAMPLES_PER_SOF * CLKS_PER_SAMPLE);
}

int feedback_samples_offset(struct feedback_ctx *ctx)
{
	int32_t offset = ctx->rel_sof_offset + ctx->base_sof_offset;

	return offset / CLKS_PER_SAMPLE;
}
