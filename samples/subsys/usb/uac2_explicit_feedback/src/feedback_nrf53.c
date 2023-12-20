/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/logging/log.h>
#include "feedback.h"

#include <nrfx_dppi.h>
#include <nrfx_gpiote.h>
#include <nrfx_timer.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_usbd.h>
#include <hal/nrf_i2s.h>
#include <helpers/nrfx_gppi.h>

LOG_MODULE_REGISTER(feedback, LOG_LEVEL_INF);

static const nrfx_gpiote_t gpiote = NRFX_GPIOTE_INSTANCE(0);

#define FEEDBACK_PIN NRF_GPIO_PIN_MAP(1, 9)
#define FEEDBACK_TIMER_INSTANCE_NUMBER 2
#define FEEDBACK_TIMER_USBD_SOF_CAPTURE 0
#define FEEDBACK_TIMER_I2S_FRAMESTART_CAPTURE 1

static const nrfx_timer_t feedback_timer_instance =
	NRFX_TIMER_INSTANCE(FEEDBACK_TIMER_INSTANCE_NUMBER);

/* See 5.12.4.2 Feedback in Universal Serial Bus Specification Revision 2.0 for
 * more information about the feedback. There is a direct implementation of the
 * specification where P=1 when @kconfig{CONFIG_APP_USE_I2S_LRCLK_EDGES_COUNTER}
 * is enabled, because I2S LRCLK edges (and not the clock) are being counted by
 * a timer. Otherwise, when @kconfig{CONFIG_APP_USE_I2S_LRCLK_EDGES_COUNTER} is
 * disabled, we are faking P=5 value using indirect offset measurements and
 * we use such estimate on PI controller updated on every SOF.
 *
 * While it might be possible to determine I2S FRAMESTART to USB SOF offset
 * entirely in software, the I2S API lacks appropriate timestamping. Therefore
 * this sample uses target-specific code to perform the measurements. Note that
 * the use of dedicated target-specific peripheral essentially eliminates
 * software scheduling jitter and it is likely that a pure software only
 * solution would require additional filtering in indirect offset measurements.
 *
 * Full-Speed isochronous feedback is Q10.10 unsigned integer left-justified in
 * the 24-bits so it has Q10.14 format. This sample application puts zeroes to
 * the 4 least significant bits (does not use the bits for extra precision).
 */
#define FEEDBACK_K		10
#if IS_ENABLED(CONFIG_APP_USE_I2S_LRCLK_EDGES_COUNTER)
#define FEEDBACK_P		1
#else
#define FEEDBACK_P		5
#endif

#define FEEDBACK_FS_SHIFT	4

static struct feedback_ctx {
	uint32_t fb_value;
	int32_t rel_sof_offset;
	int32_t base_sof_offset;
	union {
		/* For edge counting */
		struct {
			uint32_t fb_counter;
			uint16_t fb_periods;
		};
		/* For PI controller */
		int32_t integrator;
	};
} fb_ctx;

static nrfx_err_t feedback_edge_counter_setup(void)
{
	nrfx_err_t err;
	uint8_t feedback_gpiote_channel;
	uint8_t feedback_gppi_channel;
	nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = NRFX_GPIOTE_TRIGGER_TOGGLE,
		.p_in_channel = &feedback_gpiote_channel,
	};
	nrfx_gpiote_input_pin_config_t input_pin_config = {
		.p_trigger_config = &trigger_config,
	};

	/* App core is using feedback pin */
	nrf_gpio_pin_control_select(FEEDBACK_PIN, NRF_GPIO_PIN_SEL_APP);

	err = nrfx_gpiote_channel_alloc(&gpiote, &feedback_gpiote_channel);
	if (err != NRFX_SUCCESS) {
		return err;
	}

	nrfx_gpiote_input_configure(&gpiote, FEEDBACK_PIN, &input_pin_config);
	nrfx_gpiote_trigger_enable(&gpiote, FEEDBACK_PIN, false);

	/* Configure TIMER in COUNTER mode */
	const nrfx_timer_config_t cfg = {
		.frequency = NRFX_MHZ_TO_HZ(1UL),
		.mode = NRF_TIMER_MODE_COUNTER,
		.bit_width = NRF_TIMER_BIT_WIDTH_32,
		.interrupt_priority = NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
		.p_context = NULL,
	};

	err = nrfx_timer_init(&feedback_timer_instance, &cfg, NULL);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx timer init error (sample clk feedback) - Return value: %d", err);
		return err;
	}

	/* Subscribe TIMER COUNT task to GPIOTE IN event */
	err = nrfx_gppi_channel_alloc(&feedback_gppi_channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("gppi_channel_alloc failed with: %d\n", err);
		return err;
	}

	nrfx_gppi_channel_endpoints_setup(feedback_gppi_channel,
		nrfx_gpiote_in_event_address_get(&gpiote, FEEDBACK_PIN),
		nrfx_timer_task_address_get(&feedback_timer_instance, NRF_TIMER_TASK_COUNT));

	nrfx_gppi_channels_enable(BIT(feedback_gppi_channel));

	return NRFX_SUCCESS;
}

static nrfx_err_t feedback_relative_timer_setup(void)
{
	nrfx_err_t err;
	const nrfx_timer_config_t cfg = {
		.frequency = NRFX_MHZ_TO_HZ(16UL),
		.mode = NRF_TIMER_MODE_TIMER,
		.bit_width = NRF_TIMER_BIT_WIDTH_32,
		.interrupt_priority = NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
		.p_context = NULL,
	};

	err = nrfx_timer_init(&feedback_timer_instance, &cfg, NULL);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx timer init error (relative timer) - Return value: %d", err);
	}

	return err;
}

struct feedback_ctx *feedback_init(void)
{
	nrfx_err_t err;
	uint8_t usbd_sof_gppi_channel;
	uint8_t i2s_framestart_gppi_channel;

	feedback_reset_ctx(&fb_ctx);

	if (IS_ENABLED(CONFIG_APP_USE_I2S_LRCLK_EDGES_COUNTER)) {
		err = feedback_edge_counter_setup();
	} else {
		err = feedback_relative_timer_setup();
	}

	if (err != NRFX_SUCCESS) {
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

	if (!IS_ENABLED(CONFIG_APP_USE_I2S_LRCLK_EDGES_COUNTER)) {
		uint32_t clks_per_edge;

		/* Convert timer clock (independent from both Audio clock and
		 * USB host SOF clock) to fake sample clock shifted by P values.
		 * This works fine because the regulator cares only about error
		 * (SOF offset is both error and regulator input) and achieves
		 * its goal by adjusting feedback value. SOF offset is around 0
		 * when regulated and therefore the relative clock frequency
		 * discrepancies are essentially negligible.
		 */
		clks_per_edge = sof_cc / (SAMPLES_PER_SOF << FEEDBACK_P);
		sof_cc /= MAX(clks_per_edge, 1);
		framestart_cc /= MAX(clks_per_edge, 1);
	}

	/* /2 because we treat the middle as a turning point from being
	 * "too late" to "too early".
	 */
	if (framestart_cc > (SAMPLES_PER_SOF << FEEDBACK_P)/2) {
		sof_offset = framestart_cc - (SAMPLES_PER_SOF << FEEDBACK_P);
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
			base_change = -(SAMPLES_PER_SOF << FEEDBACK_P);
		} else {
			abs_diff = ctx->rel_sof_offset - sof_offset;
			base_change = SAMPLES_PER_SOF << FEEDBACK_P;
		}

		/* Adjust base offset only if the change happened through the
		 * outer bound. The actual changes should be significantly lower
		 * than the threshold here.
		 */
		if (abs_diff > (SAMPLES_PER_SOF << FEEDBACK_P)/2) {
			ctx->base_sof_offset += base_change;
		}
	}

	ctx->rel_sof_offset = sof_offset;
}

static inline int32_t offset_to_correction(int32_t offset)
{
	return -(offset / BIT(FEEDBACK_P)) * BIT(FEEDBACK_FS_SHIFT);
}

static int32_t pi_update(struct feedback_ctx *ctx)
{
	int32_t sof_offset = ctx->rel_sof_offset + ctx->base_sof_offset;
	/* SOF offset is measured in pow(2, -FEEDBACK_P) samples, i.e. when
	 * FEEDBACK_P is 0, offset is in samples, and for 1 -> half-samples,
	 * 2 -> quarter-samples, 3 -> eightth-samples and so on.
	 * In order to simplify the PI controller description here, normalize
	 * the offset to 1/1024 samples (alternatively it can be treated as
	 * samples in Q10 fixed point format) and use it as Process Variable.
	 */
	int32_t PV = BIT(10 - FEEDBACK_P) * sof_offset;
	/* The control goal is to keep I2S FRAMESTART as close as possible to
	 * USB SOF and therefore Set Point is 0.
	 */
	int32_t SP = 0;
	int32_t error = SP - PV;

	/*
	 * With above normalization at Full-Speed, when data received during
	 * SOF n appears on I2S during SOF n+3, the Ziegler Nichols Ultimate
	 * Gain is around 1.15 and the oscillation period is around 90 SOF.
	 * (much nicer oscillations with 204.8 SOF period can be observed with
	 * gain 0.5 when the delay is not n+3, but n+33 - surprisingly the
	 * resulting PI coefficients after power of two rounding are the same).
	 *
	 * Ziegler-Nichols rule with applied stability margin of 2 results in:
	 *   Kc = 0.22 * Ku = 0.22 * 1.15 = 0.253
	 *   Ti = 0.83 * tu = 0.83 * 80 = 66.4
	 *
	 * Converting the rules above to parallel PI gives:
	 *   Kp = Kc = 0.253
	 *   Ki = Kc/Ti = 0.254/66.4 ~= 0.0038253
	 *
	 * Because we want fixed-point optimized non-tunable implementation,
	 * the parameters can be conveniently expressed with power of two:
	 *   Kp ~= pow(2, -2) = 0.25    (divide by 4)
	 *   Ki ~= pow(2, -8) = 0.0039  (divide by 256)
	 *
	 * This can be implemented as:
	 *   ctx->integrator += error;
	 *   return (error + (ctx->integrator / 64)) / 4;
	 * but unfortunately such regulator is pretty aggressive and keeps
	 * oscillating rather quickly around the setpoint (within +-1 sample).
	 *
	 * Manually tweaking the constants so the regulator output is shifted
	 * down by 4 bits (i.e. change /64 to /2048 and /4 to /128) yields
	 * really good results (the outcome is similar, even slightly better,
	 * than using I2S LRCLK edge counting directly).
	 */
	ctx->integrator += error;
	return (error + (ctx->integrator / 2048)) / 128;
}

void feedback_process(struct feedback_ctx *ctx)
{
	uint32_t sof_cc;
	uint32_t framestart_cc;
	uint32_t fb;

	sof_cc = nrfx_timer_capture_get(&feedback_timer_instance,
		FEEDBACK_TIMER_USBD_SOF_CAPTURE);
	framestart_cc = nrfx_timer_capture_get(&feedback_timer_instance,
		FEEDBACK_TIMER_I2S_FRAMESTART_CAPTURE);

	update_sof_offset(ctx, sof_cc, framestart_cc);

	if (IS_ENABLED(CONFIG_APP_USE_I2S_LRCLK_EDGES_COUNTER)) {
		int32_t offset = ctx->rel_sof_offset + ctx->base_sof_offset;

		ctx->fb_counter += sof_cc;
		ctx->fb_periods++;

		if (ctx->fb_periods == BIT(FEEDBACK_K - FEEDBACK_P)) {

			/* fb_counter holds Q10.10 value, left-justify it */
			fb = ctx->fb_counter << FEEDBACK_FS_SHIFT;

			/* Align I2S FRAMESTART to USB SOF by adjusting reported
			 * feedback value. This is endpoint specific correction
			 * mentioned but not specified in USB 2.0 Specification.
			 */
			if (abs(offset) > BIT(FEEDBACK_P)) {
				fb += offset_to_correction(offset);
			}

			ctx->fb_value = fb;
			ctx->fb_counter = 0;
			ctx->fb_periods = 0;
		}
	} else {
		/* Use PI controller to generate required feedback deviation
		 * from nominal feedback value.
		 */
		fb = SAMPLES_PER_SOF << (FEEDBACK_K + FEEDBACK_FS_SHIFT);
		/* Clear the additional LSB bits in feedback value, i.e. do not
		 * use the optional extra resolution.
		 */
		fb += pi_update(ctx) & ~0xF;
		ctx->fb_value = fb;
	}
}

void feedback_reset_ctx(struct feedback_ctx *ctx)
{
	/* Reset feedback to nominal value */
	ctx->fb_value = SAMPLES_PER_SOF << (FEEDBACK_K + FEEDBACK_FS_SHIFT);
	if (IS_ENABLED(CONFIG_APP_USE_I2S_LRCLK_EDGES_COUNTER)) {
		ctx->fb_counter = 0;
		ctx->fb_periods = 0;
	} else {
		ctx->integrator = 0;
	}
}

void feedback_start(struct feedback_ctx *ctx, int i2s_blocks_queued)
{
	/* I2S data was supposed to go out at SOF, but it is inevitably
	 * delayed due to triggering I2S start by software. Set relative
	 * SOF offset value in a way that ensures that values past "half
	 * frame" are treated as "too late" instead of "too early"
	 */
	ctx->rel_sof_offset = (SAMPLES_PER_SOF << FEEDBACK_P) / 2;
	/* If there are more than 2 I2S blocks queued, use feedback regulator
	 * to correct the situation.
	 */
	ctx->base_sof_offset = (i2s_blocks_queued - 2) *
		(SAMPLES_PER_SOF << FEEDBACK_P);
}

uint32_t feedback_value(struct feedback_ctx *ctx)
{
	return ctx->fb_value;
}
