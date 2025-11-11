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
#include <helpers/nrfx_gppi.h>

LOG_MODULE_REGISTER(feedback, LOG_LEVEL_INF);

#define FEEDBACK_TIMER_USBD_SOF_CAPTURE 0
#define FEEDBACK_TIMER_I2S_FRAMESTART_CAPTURE 1

#if IS_ENABLED(CONFIG_SOC_COMPATIBLE_NRF5340_CPUAPP)

#include <hal/nrf_usbd.h>
#include <hal/nrf_i2s.h>

#define FEEDBACK_PIN NRF_GPIO_PIN_MAP(1, 9)
#define FEEDBACK_GPIOTE_INSTANCE_NUMBER 0
#define FEEDBACK_TIMER_INSTANCE_NUMBER 2
#define USB_SOF_EVENT_ADDRESS nrf_usbd_event_address_get(NRF_USBD, NRF_USBD_EVENT_SOF)
#define I2S_FRAMESTART_EVENT_ADDRESS nrf_i2s_event_address_get(NRF_I2S0, NRF_I2S_EVENT_FRAMESTART)

static inline void feedback_target_init(void)
{
	if (IS_ENABLED(CONFIG_APP_USE_I2S_LRCLK_EDGES_COUNTER)) {
		/* App core is using feedback pin */
		nrf_gpio_pin_control_select(FEEDBACK_PIN, NRF_GPIO_PIN_SEL_APP);
	}
}

#elif IS_ENABLED(CONFIG_SOC_SERIES_NRF54HX)

#include <hal/nrf_tdm.h>

#define FEEDBACK_PIN NRF_GPIO_PIN_MAP(0, 8)
#define FEEDBACK_GPIOTE_INSTANCE_NUMBER 130
#define FEEDBACK_TIMER_INSTANCE_NUMBER 131
#define USB_SOF_EVENT_ADDRESS nrf_timer_event_address_get(NRF_TIMER131, NRF_TIMER_EVENT_COMPARE5)
#define I2S_FRAMESTART_EVENT_ADDRESS nrf_tdm_event_address_get(NRF_TDM130, NRF_TDM_EVENT_MAXCNT)

static inline void feedback_target_init(void)
{
	/* Enable Start-of-Frame workaround in TIMER131 */
	*(volatile uint32_t *)0x5F9A3C04 = 0x00000002;
	*(volatile uint32_t *)0x5F9A3C04 = 0x00000003;
	*(volatile uint32_t *)0x5F9A3C80 = 0x00000082;
}

#else
#error "Unsupported target"
#endif

static const nrfx_gpiote_t gpiote =
	NRFX_GPIOTE_INSTANCE(FEEDBACK_GPIOTE_INSTANCE_NUMBER);

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
 *
 * High-Speed isochronous feedback is Q12.13 unsigned integer aligned in the
 * 32-bits so the binary point is located between second and third byte so it
 * has Q16.16 format. This sample applications puts zeroes to the 3 least
 * significant bits (does not use the bits for extra precision).
 */
#define FEEDBACK_FS_K		10
#define FEEDBACK_HS_K		13
#if defined(CONFIG_APP_USE_I2S_LRCLK_EDGES_COUNTER)
#define FEEDBACK_P		1
#else
#define FEEDBACK_P		5
#endif

#define FEEDBACK_FS_SHIFT	4
#define FEEDBACK_HS_SHIFT	3

static struct feedback_ctx {
	uint32_t fb_value;
	int32_t rel_sof_offset;
	int32_t base_sof_offset;
	uint32_t counts_per_sof;
	bool high_speed;
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
	nrf_gpio_pin_pull_t pull = NRF_GPIO_PIN_PULLUP;
	nrfx_gpiote_input_pin_config_t input_pin_config = {
		.p_pull_config = &pull,
		.p_trigger_config = &trigger_config,
	};

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

	feedback_target_init();

	feedback_reset_ctx(&fb_ctx, false);

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
		USB_SOF_EVENT_ADDRESS,
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
		I2S_FRAMESTART_EVENT_ADDRESS,
		nrfx_timer_capture_task_address_get(&feedback_timer_instance,
			FEEDBACK_TIMER_I2S_FRAMESTART_CAPTURE));

	nrfx_gppi_channels_enable(BIT(i2s_framestart_gppi_channel));

	/* Enable feedback timer */
	nrfx_timer_enable(&feedback_timer_instance);

	return &fb_ctx;
}

static uint32_t nominal_feedback_value(struct feedback_ctx *ctx)
{
	if (ctx->high_speed) {
		return (SAMPLE_RATE / 8000) << (FEEDBACK_HS_K + FEEDBACK_HS_SHIFT);
	}

	return (SAMPLE_RATE / 1000) << (FEEDBACK_FS_K + FEEDBACK_FS_SHIFT);
}

static uint32_t feedback_period(struct feedback_ctx *ctx)
{
	if (ctx->high_speed) {
		return BIT(FEEDBACK_HS_K - FEEDBACK_P);
	}

	return BIT(FEEDBACK_FS_K - FEEDBACK_P);
}

static void update_sof_offset(struct feedback_ctx *ctx, uint32_t sof_cc,
			      uint32_t framestart_cc)
{
	int sof_offset;

	if (!IS_ENABLED(CONFIG_APP_USE_I2S_LRCLK_EDGES_COUNTER)) {
		uint32_t nominator;

		/* Convert timer clock (independent from both Audio clock and
		 * USB host SOF clock) to fake sample clock shifted by P values.
		 * This works fine because the regulator cares only about error
		 * (SOF offset is both error and regulator input) and achieves
		 * its goal by adjusting feedback value. SOF offset is around 0
		 * when regulated and therefore the relative clock frequency
		 * discrepancies are essentially negligible.
		 */
		nominator = MIN(sof_cc, framestart_cc) * ctx->counts_per_sof;
		sof_offset = nominator / MAX(sof_cc, 1);
	} else {
		sof_offset = framestart_cc;
	}

	/* /2 because we treat the middle as a turning point from being
	 * "too late" to "too early".
	 */
	if (sof_offset > ctx->counts_per_sof/2) {
		sof_offset -= ctx->counts_per_sof;
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
			base_change = -ctx->counts_per_sof;
		} else {
			abs_diff = ctx->rel_sof_offset - sof_offset;
			base_change = ctx->counts_per_sof;
		}

		/* Adjust base offset only if the change happened through the
		 * outer bound. The actual changes should be significantly lower
		 * than the threshold here.
		 */
		if (abs_diff > ctx->counts_per_sof/2) {
			ctx->base_sof_offset += base_change;
		}
	}

	ctx->rel_sof_offset = sof_offset;
}

static inline int32_t offset_to_correction(struct feedback_ctx *ctx, int32_t offset)
{
	if (ctx->high_speed) {
		return -(offset / BIT(FEEDBACK_P)) * BIT(FEEDBACK_HS_SHIFT);
	}

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
	 * With above normalization, when data received during SOF n appears on
	 * I2S during SOF n+3, the Ziegler Nichols Ultimate Gain and oscillation
	 * periods are as follows:
	 *
	 *   Full-Speed Linux:   Ku = 1.34   tu=77   [FS SOFs]
	 *   Full-Speed Mac OS:  Ku = 0.173  tu=580  [FS SOFs]
	 *   High-Speed Mac OS:  Ku = 0.895  tu=4516 [HS SOFs]
	 *   High-Speed Windows: Ku = 0.515  tu=819  [HS SOFs]
	 *
	 * Linux and Mac OS oscillations were very neat, while Windows seems to
	 * be averaging feedback value and therefore it is hard to get steady
	 * oscillations without getting buffer uderrun.
	 *
	 * Ziegler-Nichols rule with applied stability margin of 2 results in:
	 *                    [FS Linux] [FS Mac] [HS Mac] [HS Windows]
	 *   Kc = 0.22 * Ku    0.2948     0.0381   0.1969   0.1133
	 *   Ti = 0.83 * tu    63.91      481.4    3748     647.9
	 *
	 * Converting the rules to work with following simple regulator:
	 *   ctx->integrator += error;
	 *   return (error + (ctx->integrator / Ti)) / invKc;
	 *
	 * gives following parameters:
	 *                    [FS Linux] [FS Mac] [HS Mac] [HS Windows]
	 *   invKc = 1/Kc      3          26       5        8
	 *   Ti                64         482      3748     648
	 *
	 * The respective regulators seem to give quarter-amplitude-damping on
	 * respective hosts, but tuning from one host can get into oscillations
	 * on another host. The regulation goal is to achieve a single set of
	 * parameters to be used with all hosts, the only parameter difference
	 * can be based on operating speed.
	 *
	 * After a number of tests with all the hosts, following parameters
	 * were determined to result in nice no-overshoot response:
	 *               [Full-Speed]    [High-Speed]
	 *   invKc        128             128
	 *   Ti           2048            16384
	 *
	 * The power-of-two parameters were arbitrarily chosen for rounding.
	 * The 16384 = 2048 * 8 can be considered as unifying integration time.
	 *
	 * While the no-overshoot is also present with invKc as low as 32, such
	 * regulator is pretty aggressive and keeps oscillating rather quickly
	 * around the setpoint (within +-1 sample). Lowering the controller gain
	 * (increasing invKc value) yields really good results (the outcome is
	 * similar to using I2S LRCLK edge counting directly).
	 *
	 * The most challenging scenario is for the regulator to stabilize right
	 * after startup when I2S consumes data faster than nominal sample rate
	 * (48 kHz = 6 samples per SOF at High-Speed, 48 samples at Full-Speed)
	 * according to host (I2S consuming data slower slower than nominal
	 * sample rate is not problematic at all because buffer overrun does not
	 * stop I2S streaming). This regulator should be able to stabilize for
	 * any frequency that is within required USB SOF accuracy of 500 ppm,
	 * i.e. when nominal sample rate is 48 kHz the real sample rate can be
	 * anywhere in [47.976 kHz; 48.024 kHz] range.
	 */
	ctx->integrator += error;
	return (error + (ctx->integrator / (ctx->high_speed ? 16384 : 2048))) / 128;
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

		if (ctx->fb_periods == feedback_period(ctx)) {

			if (ctx->high_speed) {
				fb = ctx->fb_counter << FEEDBACK_HS_SHIFT;
			} else {
				/* fb_counter holds Q10.10 value, left-justify it */
				fb = ctx->fb_counter << FEEDBACK_FS_SHIFT;
			}

			/* Align I2S FRAMESTART to USB SOF by adjusting reported
			 * feedback value. This is endpoint specific correction
			 * mentioned but not specified in USB 2.0 Specification.
			 */
			if (abs(offset) > BIT(FEEDBACK_P)) {
				fb += offset_to_correction(ctx, offset);
			}

			ctx->fb_value = fb;
			ctx->fb_counter = 0;
			ctx->fb_periods = 0;
		}
	} else {
		const uint32_t zero_lsb_mask = ctx->high_speed ? 0x7 : 0xF;

		/* Use PI controller to generate required feedback deviation
		 * from nominal feedback value.
		 */
		fb = nominal_feedback_value(ctx);
		/* Clear the additional LSB bits in feedback value, i.e. do not
		 * use the optional extra resolution.
		 */
		fb += pi_update(ctx) & ~zero_lsb_mask;
		ctx->fb_value = fb;
	}
}

void feedback_reset_ctx(struct feedback_ctx *ctx, bool microframes)
{
	/* Reset feedback to nominal value */
	ctx->high_speed = microframes;
	ctx->fb_value = nominal_feedback_value(ctx);
	if (IS_ENABLED(CONFIG_APP_USE_I2S_LRCLK_EDGES_COUNTER)) {
		ctx->fb_counter = 0;
		ctx->fb_periods = 0;
	} else {
		ctx->integrator = 0;
	}
}

void feedback_start(struct feedback_ctx *ctx, int i2s_blocks_queued,
		    bool microframes)
{
	ctx->high_speed = microframes;
	ctx->fb_value = nominal_feedback_value(ctx);

	if (microframes) {
		ctx->counts_per_sof = (SAMPLE_RATE / 8000) << FEEDBACK_P;
	} else {
		ctx->counts_per_sof = (SAMPLE_RATE / 1000) << FEEDBACK_P;
	}

	/* I2S data was supposed to go out at SOF, but it is inevitably
	 * delayed due to triggering I2S start by software. Set relative
	 * SOF offset value in a way that ensures that values past "half
	 * frame" are treated as "too late" instead of "too early"
	 */
	ctx->rel_sof_offset = ctx->counts_per_sof / 2;
	/* If there are more than 2 I2S blocks queued, use feedback regulator
	 * to correct the situation.
	 */
	ctx->base_sof_offset = (i2s_blocks_queued - 2) * ctx->counts_per_sof;
}

uint32_t feedback_value(struct feedback_ctx *ctx)
{
	return ctx->fb_value;
}
