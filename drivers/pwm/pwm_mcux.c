/*
 * Copyright (c) 2019, Linaro
 * Copyright 2025-2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_pwm

#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/pm/device.h>
#include <soc.h>
#include <fsl_flexpwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_PWM_CAPTURE
#include <zephyr/irq.h>
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_mcux, CONFIG_PWM_LOG_LEVEL);

#define CHANNEL_COUNT 3

/* Order of the nxp,reload strings in the binding. */
#define RELOAD_IMMEDIATE           0U
#define RELOAD_HALF_CYCLE          1U
#define RELOAD_FULL_CYCLE          2U
#define RELOAD_HALF_AND_FULL_CYCLE 3U

/* Which output state mcux_pwm_config_channels() programs. */
enum mcux_pwm_output_state {
	MCUX_PWM_OUTPUT_REQUESTED, /* the waveform the caller asked for */
	MCUX_PWM_OUTPUT_INACTIVE,  /* everything parked, for suspend */
};

struct pwm_mcux_channel {
	uint32_t pulse_cycles;
	pwm_flags_t flags;
	bool configured;
	/* Mirrors the MASK bit, so an unchanged mask needs no commit. */
	bool masked;
};

#ifdef CONFIG_PWM_CAPTURE
struct pwm_mcux_capture_data {
	pwm_capture_callback_handler_t callback;
	void *user_data;
	/* Counter values in one cycle: VAL1 - INIT + 1. */
	uint32_t modulo;
	/* Counter cycles since the first edge, counted by the reload interrupt. */
	uint32_t reload_count;
	uint32_t capture_channel;
	bool continuous : 1;
	bool pulse_capture : 1;
};
#endif /* CONFIG_PWM_CAPTURE */

struct pwm_mcux_config {
	PWM_Type *base;
	uint8_t index;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	flexpwm_prescaler_t prescale;
	uint8_t reload;
	bool run_wait;
	bool run_debug;
	const struct pinctrl_dev_config *pincfg;
#ifdef CONFIG_PWM_CAPTURE
	uint8_t input_filter_count;
	uint8_t input_filter_period;
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct pwm_mcux_data {
	uint32_t clock_freq;
	/* A submodule has one counter, so all three channels share the period. */
	uint32_t period_cycles;
	struct pwm_mcux_channel channel[CHANNEL_COUNT];
	struct k_mutex lock;
#ifdef CONFIG_PWM_CAPTURE
	struct pwm_mcux_capture_data capture;
	bool capture_active;
#endif
};

static inline uint16_t mcux_pwm_submodule_mask(const struct device *dev)
{
	const struct pwm_mcux_config *config = dev->config;

	return (uint16_t)BIT(config->index);
}

#ifdef CONFIG_PM_DEVICE
static bool mcux_pwm_any_channel_configured(const struct pwm_mcux_data *data)
{
	uint32_t channel;

	for (channel = 0; channel < CHANNEL_COUNT; channel++) {
		if (data->channel[channel].configured) {
			return true;
		}
	}

	return false;
}
#endif /* CONFIG_PM_DEVICE */

/*
 * 0% and 100% duty are static levels, and the output mask is what holds them. Masking
 * forces a channel to 0 ahead of the polarity stage, so a masked pin follows OCTRL
 * POLx - and POLx names the active level, so its own bit value is the inactive one.
 * At 0% that is all it takes: mask the channel and leave POLx alone.
 *
 * 100% is free on A and B. Their off edge is VAL3/VAL5, full width puts it at period,
 * and the counter stops at period - 1, so the compare never matches and the output
 * stays on. X has only VAL0, where full width would need VAL0 = -1. So X gets masked
 * too, with POLx inverted to hold the pin active instead.
 */
static bool mcux_pwm_full_width_needs_mask(const struct pwm_mcux_data *data, uint32_t channel)
{
	return (channel == 2U) && data->channel[channel].configured &&
	       (data->channel[channel].pulse_cycles >= data->period_cycles);
}

static bool mcux_pwm_static_level(const struct pwm_mcux_data *data, uint32_t channel)
{
	return (data->channel[channel].configured &&
		(data->channel[channel].pulse_cycles == 0U)) ||
	       mcux_pwm_full_width_needs_mask(data, channel);
}

static flexpwm_pwm_polarity_t mcux_pwm_polarity(const struct pwm_mcux_data *data,
						uint32_t channel,
						enum mcux_pwm_output_state state)
{
	bool inverted = (data->channel[channel].flags & PWM_POLARITY_INVERTED) != 0U;

	/* Channel X at full width is the one case where the mask has to hold the pin
	 * active rather than inactive, so flip POLx there.
	 */
	if ((state == MCUX_PWM_OUTPUT_REQUESTED) &&
	    mcux_pwm_full_width_needs_mask(data, channel)) {
		inverted = !inverted;
	}

	/* For a masked channel this is the level on the pin, not a polarity - the enum
	 * name reads backwards there.
	 */
	return inverted ? kFLEXPWM_Polarity_ActiveLow : kFLEXPWM_Polarity_ActiveHigh;
}

/*
 * MASK is double buffered: a write does nothing until a FORCE_OUT event. Most parts
 * have MASK[UPDATE_MASK] to skip that. The rest need a software force, which is what
 * CTRL2[FORCE_SEL] selects at its reset value.
 */
static void mcux_pwm_commit_mask(const struct device *dev)
{
	const struct pwm_mcux_config *config = dev->config;

#if defined(FSL_FEATURE_PWM_MASK_HAS_UPDATE_MASK_BITFIELD) && \
	(FSL_FEATURE_PWM_MASK_HAS_UPDATE_MASK_BITFIELD == 1U)
	FLEXPWM_EnableUpdateMaskImmediately(config->base, mcux_pwm_submodule_mask(dev));
#else
	FLEXPWM_SetLocalForceOut(config->base, config->index);
#endif
}

static void mcux_pwm_mask_channel(const struct device *dev, uint32_t channel, bool masked)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	uint16_t submodule = mcux_pwm_submodule_mask(dev);
	uint16_t a_mask = (channel == 0U) ? submodule : 0U;
	uint16_t b_mask = (channel == 1U) ? submodule : 0U;
	uint16_t x_mask = (channel == 2U) ? submodule : 0U;

	/* A commit may mean a FORCE_OUT event, and that reinitializes the outputs.
	 * Skip it when the mask has not changed.
	 */
	if (data->channel[channel].masked == masked) {
		return;
	}

	if (masked) {
		FLEXPWM_MaskPWMOutput(config->base, a_mask, b_mask, x_mask);
	} else {
		FLEXPWM_UnMaskPWMOutput(config->base, a_mask, b_mask, x_mask);
	}

	mcux_pwm_commit_mask(dev);

	data->channel[channel].masked = masked;
}

static void mcux_pwm_output_channel(const struct device *dev, uint32_t channel, bool enable)
{
	const struct pwm_mcux_config *config = dev->config;
	uint16_t submodule = mcux_pwm_submodule_mask(dev);
	uint16_t a_mask = (channel == 0U) ? submodule : 0U;
	uint16_t b_mask = (channel == 1U) ? submodule : 0U;
	uint16_t x_mask = (channel == 2U) ? submodule : 0U;

	if (enable) {
		FLEXPWM_EnablePWMOutput(config->base, a_mask, b_mask, x_mask);
	} else {
		FLEXPWM_DisablePWMOutput(config->base, a_mask, b_mask, x_mask);
	}
}

static bool mcux_pwm_half_cycle_reload(uint8_t reload)
{
	return (reload == RELOAD_HALF_CYCLE) || (reload == RELOAD_HALF_AND_FULL_CYCLE);
}

static void mcux_pwm_reload_config(uint8_t reload, flexpwm_reload_config_t *reload_config)
{
	reload_config->loadMode = (reload == RELOAD_IMMEDIATE) ? kFLEXPWM_LoadMode_Immediate
							       : kFLEXPWM_LoadMode_Opportunity;
	reload_config->enableHalfCycleReload = mcux_pwm_half_cycle_reload(reload);
	reload_config->enableFullCycleReload = (reload == RELOAD_FULL_CYCLE) ||
					       (reload == RELOAD_HALF_AND_FULL_CYCLE);
}

static void mcux_pwm_submodule_defaults(const struct device *dev,
					flexpwm_submodule_config_t *submodule_config)
{
	const struct pwm_mcux_config *config = dev->config;

	/* Clock source is the IPBus clock, and initValue is 0, which channel X's VAL0 and the
	 * capture modulo both assume.
	 */
	FLEXPWM_GetDefaultSubmoduleConfig(submodule_config);

	submodule_config->prescaler = config->prescale;
	submodule_config->enableDebugMode = config->run_debug;
#if !(defined(FSL_FEATURE_PWM_HAS_NO_WAITEN) && (FSL_FEATURE_PWM_HAS_NO_WAITEN == 1U))
	/* A submodule that stops in Wait mode also stops capturing, so this is not
	 * only about waveform output.
	 */
	submodule_config->enableWaitMode = config->run_wait;
#endif
}

static void mcux_pwm_config_counter(const struct device *dev)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	flexpwm_submodule_config_t submodule_config;

	mcux_pwm_submodule_defaults(dev, &submodule_config);

	submodule_config.counterConfig.modValue = (uint16_t)(data->period_cycles - 1U);

	mcux_pwm_reload_config(config->reload, &submodule_config.reloadConfig);
	submodule_config.reloadConfig.halfCycleValue = (uint16_t)(data->period_cycles / 2U);

	FLEXPWM_ConfigSubmodule(config->base, config->index, &submodule_config);
}

/*
 * A and B each get a pair of compares: active on VAL2/VAL4, inactive on VAL3/VAL5.
 * VAL2/VAL4 stay at 0 and the width goes in VAL3/VAL5, so every pulse starts on the
 * period boundary - that is what makes A and B edge aligned.
 *
 * X only gets VAL0. Its off edge is VAL1, which is also the counter modulo, so an X
 * pulse always ends on the period boundary and its width is VAL1 - VAL0. VAL1 is
 * period - 1 already, so VAL0 goes one lower.
 */
static void mcux_pwm_config_channels(const struct device *dev, enum mcux_pwm_output_state state)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	flexpwm_pwm_config_t pwm_config;
	flexpwm_pwm_channel_config_t pwmx_config;

	pwm_config.complementary = false;
	pwm_config.ipolSource = kFLEXPWM_IPOL_PWM23;

	pwm_config.pwma.compareValue_ON = 0U;
	pwm_config.pwma.compareValue_OFF = (uint16_t)data->channel[0].pulse_cycles;
	pwm_config.pwma.polarity = mcux_pwm_polarity(data, 0, state);

	pwm_config.pwmb.compareValue_ON = 0U;
	pwm_config.pwmb.compareValue_OFF = (uint16_t)data->channel[1].pulse_cycles;
	pwm_config.pwmb.polarity = mcux_pwm_polarity(data, 1, state);

	FLEXPWM_ConfigPWM(config->base, config->index, &pwm_config);

	/* VAL0 doubles as the half cycle reload point. Leave it alone until channel X
	 * is actually in use.
	 */
	if (!data->channel[2].configured) {
		return;
	}

	if (mcux_pwm_full_width_needs_mask(data, 2)) {
		/* VAL0 cannot reach this width, so the mask is holding the level. Set
		 * VAL0 to the width that generates nothing, so unmasking later gives
		 * no pulse.
		 */
		pwmx_config.compareValue_ON = (uint16_t)(data->period_cycles - 1U);
	} else {
		pwmx_config.compareValue_ON =
			(uint16_t)(data->period_cycles - 1U - data->channel[2].pulse_cycles);
	}
	pwmx_config.compareValue_OFF = (uint16_t)(data->period_cycles - 1U);
	pwmx_config.polarity = mcux_pwm_polarity(data, 2, state);

	FLEXPWM_ConfigPWMChannelX(config->base, config->index, &pwmx_config);
}

/* Write the cached setup of every channel to the hardware. */
static void mcux_pwm_program(const struct device *dev, bool reprogram_counter)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	uint32_t channel;

	if (reprogram_counter) {
		/* A shorter modulo can leave the counter past its new end. The outputs
		 * then hold for a whole wrap before the next reload.
		 */
		FLEXPWM_DisableSubmoduleCounter(config->base, mcux_pwm_submodule_mask(dev));
	}

	/* VALx is double buffered and cannot be written while LDOK is set. Clear LDOK,
	 * write the whole submodule, set LDOK again.
	 */
	FLEXPWM_ClearLoadOkay(config->base, mcux_pwm_submodule_mask(dev));

	if (reprogram_counter) {
		mcux_pwm_config_counter(dev);
	}

	mcux_pwm_config_channels(dev, MCUX_PWM_OUTPUT_REQUESTED);

	FLEXPWM_SetLoadOkay(config->base, mcux_pwm_submodule_mask(dev));

	/* OUTEN and MASK are not buffered, so these land immediately. */
	for (channel = 0; channel < CHANNEL_COUNT; channel++) {
		if (!data->channel[channel].configured) {
			continue;
		}

		mcux_pwm_mask_channel(dev, channel, mcux_pwm_static_level(data, channel));
		mcux_pwm_output_channel(dev, channel, true);
	}

	FLEXPWM_EnableSubmoduleCounter(config->base, mcux_pwm_submodule_mask(dev));
}

static int mcux_pwm_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	struct pwm_mcux_data *data = dev->data;
	const struct pwm_mcux_config *config = dev->config;
	bool period_changed;
	uint32_t other;

	if (channel >= CHANNEL_COUNT) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (period_cycles == 0U) {
		LOG_ERR("zero period, use pulse = 0 for a constant inactive level");
		return -ENOTSUP;
	}

	if (period_cycles > UINT16_MAX) {
		/* 16-bit resolution */
		LOG_ERR("Too long period (%u), adjust pwm prescaler!", period_cycles);
		/* TODO: dynamically adjust prescaler */
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

#ifdef CONFIG_PWM_CAPTURE
	if (data->capture_active) {
		/* Reprogramming moves the time base the capture is measured against,
		 * and a new period stops the counter outright.
		 */
		LOG_ERR("PWM capture is active, cannot set PWM output");
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}
#endif

	period_changed = (period_cycles != data->period_cycles);

	if ((channel == 2U) && !data->channel[channel].configured &&
	    mcux_pwm_half_cycle_reload(config->reload)) {
		LOG_WRN("Channel X uses VAL0, so the half cycle reload point follows its "
			"duty instead of mid period.");
	}

	data->channel[channel].pulse_cycles = pulse_cycles;
	data->channel[channel].flags = flags;
	data->channel[channel].configured = true;

	if (period_changed) {
		/* One counter, so a new period hits every channel. The others keep the
		 * pulse width they asked for, which means their duty changes.
		 */
		for (other = 0; other < CHANNEL_COUNT; other++) {
			if ((other == channel) || !data->channel[other].configured) {
				continue;
			}

			LOG_WRN("channel %u now runs at the period of channel %u", other,
				channel);
			data->channel[other].pulse_cycles =
				MIN(data->channel[other].pulse_cycles, period_cycles);
		}

		data->period_cycles = period_cycles;
	}

	mcux_pwm_program(dev, period_changed);

	k_mutex_unlock(&data->lock);

	return 0;
}

static int mcux_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
				       uint64_t *cycles)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;

	*cycles = data->clock_freq >> config->prescale;

	return 0;
}

#ifdef CONFIG_PWM_CAPTURE

/* CVALxCYC is four bits wide. */
#define CAPTURE_CYCLE_MASK  ((uint32_t)PWM_CVAL0CYC_CVAL0CYC_MASK)
#define CAPTURE_CYCLE_RANGE (CAPTURE_CYCLE_MASK + 1U)

/* Entries in a capture FIFO. */
#define CAPTURE_FIFO_DEPTH 4U

static flexpwm_capture_channel_t mcux_pwm_capture_channel(uint32_t channel)
{
	switch (channel) {
	case 0U:
		return kFLEXPWM_Capture_A;
	case 1U:
		return kFLEXPWM_Capture_B;
	default:
		return kFLEXPWM_Capture_X;
	}
}

/* Capture_X lands on CVAL0/1, Capture_A on CVAL2/3 and Capture_B on CVAL4/5. */
static flexpwm_capture_index_t mcux_pwm_capture_index(uint32_t channel, bool second_edge)
{
	switch (channel) {
	case 0U:
		return second_edge ? kFLEXPWM_Capture_A_Edge1 : kFLEXPWM_Capture_A_Edge0;
	case 1U:
		return second_edge ? kFLEXPWM_Capture_B_Edge1 : kFLEXPWM_Capture_B_Edge0;
	default:
		return second_edge ? kFLEXPWM_Capture_X_Edge1 : kFLEXPWM_Capture_X_Edge0;
	}
}

static flexpwm_capture_channel_config_t *
mcux_pwm_capture_channel_config(flexpwm_input_capture_config_t *config, uint32_t channel)
{
	switch (channel) {
	case 0U:
		return &config->captureA;
	case 1U:
		return &config->captureB;
	default:
		return &config->captureX;
	}
}

static uint16_t mcux_pwm_capture_irq_mask(uint32_t channel)
{
	switch (channel) {
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELA) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELA == 1U)
	case 0U:
		return (uint16_t)(kFLEXPWM_CaptureA0InterruptEnable |
				  kFLEXPWM_CaptureA1InterruptEnable |
				  kFLEXPWM_ReloadInterruptEnable);
#endif
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELB) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELB == 1U)
	case 1U:
		return (uint16_t)(kFLEXPWM_CaptureB0InterruptEnable |
				  kFLEXPWM_CaptureB1InterruptEnable |
				  kFLEXPWM_ReloadInterruptEnable);
#endif
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELX) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELX == 1U)
	case 2U:
		return (uint16_t)(kFLEXPWM_CaptureX0InterruptEnable |
				  kFLEXPWM_CaptureX1InterruptEnable |
				  kFLEXPWM_ReloadInterruptEnable);
#endif
	default:
		/* unsupported channel for this SoC: check_channel() rejected it */
		return 0U;
	}
}

static uint16_t mcux_pwm_capture_status_flag(uint32_t channel, bool second_edge)
{
	switch (channel) {
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELA) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELA == 1U)
	case 0U:
		return (uint16_t)(second_edge ? kFLEXPWM_CaptureA1Flag : kFLEXPWM_CaptureA0Flag);
#endif
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELB) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELB == 1U)
	case 1U:
		return (uint16_t)(second_edge ? kFLEXPWM_CaptureB1Flag : kFLEXPWM_CaptureB0Flag);
#endif
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELX) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELX == 1U)
	case 2U:
		return (uint16_t)(second_edge ? kFLEXPWM_CaptureX1Flag : kFLEXPWM_CaptureX0Flag);
#endif
	default:
		/* unsupported channel for this SoC: check_channel() rejected it */
		return 0U;
	}
}

static void mcux_pwm_capture_irq_disable(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_config *config = dev->config;

	FLEXPWM_DisableSubmoduleInterrupts(config->base, config->index,
					   mcux_pwm_capture_irq_mask(channel));
}

/*
 * CVALxCYC latches the counter cycle each edge was captured in, so both timestamps
 * come out of the hardware ready to subtract - no software counting between the edges.
 *
 * The catch: the field is only four bits, and a slow input wraps it. So the reload
 * interrupt also keeps a wide cycle count from the first edge. That count can be off
 * by one, because a reload and a capture can land on either side of the ISR, and the
 * low nibble from CVALxCYC is what corrects it.
 */
static uint32_t mcux_pwm_capture_cycles(uint16_t first_cycle, uint16_t second_cycle,
					uint32_t counted)
{
	uint32_t low = ((uint32_t)second_cycle - (uint32_t)first_cycle) & CAPTURE_CYCLE_MASK;
	int32_t adjust = (int32_t)low - (int32_t)(counted & CAPTURE_CYCLE_MASK);

	if (adjust > (int32_t)(CAPTURE_CYCLE_RANGE / 2U)) {
		adjust -= (int32_t)CAPTURE_CYCLE_RANGE;
	} else if (adjust < -(int32_t)(CAPTURE_CYCLE_RANGE / 2U)) {
		adjust += (int32_t)CAPTURE_CYCLE_RANGE;
	}

	if ((adjust < 0) && ((uint32_t)(-adjust) > counted)) {
		/* No usable software count yet, so use the hardware value as is. */
		return low;
	}

	return counted + (uint32_t)adjust;
}

static int mcux_pwm_capture_span(uint16_t first_value, uint16_t second_value, uint32_t cycles,
				uint32_t modulo, uint32_t *ticks)
{
	uint32_t span;

	/* An edge is at cycles * modulo + counter value. Subtracting two of those
	 * needs no wrap correction.
	 */
	if (u32_mul_overflow(cycles, modulo, &span)) {
		LOG_ERR("captured interval does not fit in 32 bits");
		return -ERANGE;
	}

	if (u32_add_overflow(span, (uint32_t)second_value, &span)) {
		LOG_ERR("captured interval does not fit in 32 bits");
		return -ERANGE;
	}

	*ticks = span - (uint32_t)first_value;

	return 0;
}

static void mcux_pwm_handle_capture(const struct device *dev)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	struct pwm_mcux_capture_data *capture = &data->capture;
	flexpwm_capture_index_t first = mcux_pwm_capture_index(capture->capture_channel, false);
	flexpwm_capture_index_t second = mcux_pwm_capture_index(capture->capture_channel, true);
	uint16_t first_cycle, second_cycle, first_value, second_value;
	uint32_t cycles;
	uint32_t ticks = 0U;
	int err;

	/* Read the cycle registers first. A CVAL read pops the FIFO, and CVALxCYC
	 * would then describe the next sample.
	 */
	first_cycle = FLEXPWM_GetInputCaptureCycle(config->base, config->index, first);
	second_cycle = FLEXPWM_GetInputCaptureCycle(config->base, config->index, second);
	first_value = FLEXPWM_GetInputCaptureValue(config->base, config->index, first);
	second_value = FLEXPWM_GetInputCaptureValue(config->base, config->index, second);

	cycles = mcux_pwm_capture_cycles(first_cycle, second_cycle, capture->reload_count);
	err = mcux_pwm_capture_span(first_value, second_value, cycles, capture->modulo, &ticks);

	LOG_DBG("capture %u+%u -> %u+%u, %u cycles, %u ticks", first_cycle, first_value,
		second_cycle, second_value, cycles, ticks);

	if (capture->pulse_capture) {
		capture->callback(dev, capture->capture_channel, 0, ticks, err,
				  capture->user_data);
	} else {
		capture->callback(dev, capture->capture_channel, ticks, 0, err,
				  capture->user_data);
	}

	if (!capture->continuous) {
		/* One shot is done. The hardware disarms itself, so drop the interrupts
		 * that would otherwise keep firing on every reload. The channel is still
		 * ours until pwm_disable_capture() - that is what the API expects.
		 */
		mcux_pwm_capture_irq_disable(dev, capture->capture_channel);
		pm_device_busy_clear(dev);
	}
}

static void mcux_pwm_isr(const struct device *dev)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	struct pwm_mcux_capture_data *capture = &data->capture;
	uint32_t channel = capture->capture_channel;
	uint16_t status;

	status = FLEXPWM_GetSubmoduleStatusFlags(config->base, config->index);

	if ((status & (uint16_t)kFLEXPWM_ReloadFlag) != 0U) {
		capture->reload_count++;
	}

	if ((status & mcux_pwm_capture_status_flag(channel, false)) != 0U) {
		/* The first edge starts the measurement, so restart the cycle count. */
		capture->reload_count = 0U;
	}

	if ((status & mcux_pwm_capture_status_flag(channel, true)) != 0U) {
		mcux_pwm_handle_capture(dev);
	}

	FLEXPWM_ClearSubmoduleStatusFlags(config->base, config->index, status);
}

static int check_channel(const struct device *dev, uint32_t channel)
{
	struct pwm_mcux_data *data = dev->data;

	if (channel >= CHANNEL_COUNT) {
		LOG_ERR("Invalid channel %u", channel);
		return -EINVAL;
	}

	if (data->channel[channel].configured) {
		LOG_ERR("Channel %u is already used for PWM output", channel);
		return -EBUSY;
	}

	if (channel == 0U) {
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELA) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELA == 0U)
		LOG_ERR("Channel A does not support capture on this hardware");
		return -ENOTSUP;
#endif
	} else if (channel == 1U) {
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELB) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELB == 0U)
		LOG_ERR("Channel B does not support capture on this hardware");
		return -ENOTSUP;
#endif
	} else {
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELX) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELX == 0U)
		LOG_ERR("Channel X does not support capture on this hardware");
		return -ENOTSUP;
#endif
	}

	return 0;
}

/*
 * mcux_pwm_handle_capture() needs the counter modulo to turn timestamps into ticks,
 * so a submodule that generates nothing still needs a time base. The free running
 * 16-bit range and the full cycle reload come from mcux_pwm_submodule_defaults().
 * Only immediate load mode is left to add: a stopped counter never reaches a reload
 * opportunity, so INIT and VAL1 would otherwise never load.
 */
static void mcux_pwm_config_capture_counter(const struct device *dev)
{
	const struct pwm_mcux_config *config = dev->config;
	flexpwm_submodule_config_t submodule_config;

	mcux_pwm_submodule_defaults(dev, &submodule_config);

	submodule_config.reloadConfig.loadMode = kFLEXPWM_LoadMode_Immediate;
	submodule_config.reloadConfig.halfCycleValue = 0U;

	FLEXPWM_ConfigSubmodule(config->base, config->index, &submodule_config);
	FLEXPWM_SetLoadOkay(config->base, mcux_pwm_submodule_mask(dev));
}

#if defined(FSL_FEATURE_PWM_HAS_INPUT_FILTER_CAPTURE) && \
	(FSL_FEATURE_PWM_HAS_INPUT_FILTER_CAPTURE == 1U)
static void mcux_pwm_config_capture_filter(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_config *config = dev->config;
	flexpwm_capture_filter_config_t filter_config;
	flexpwm_capture_filter_channel_config_t *channel_filter;

	FLEXPWM_GetDefaultCaptureFilterConfig(&filter_config);

	switch (channel) {
	case 0U:
		channel_filter = &filter_config.captureA;
		break;
	case 1U:
		channel_filter = &filter_config.captureB;
		break;
	default:
		channel_filter = &filter_config.captureX;
		break;
	}

	channel_filter->filterPeriod = config->input_filter_period;
	channel_filter->filterCount = config->input_filter_count;

	FLEXPWM_ConfigInputCaptureFilter(config->base, config->index, &filter_config);
}
#endif /* FSL_FEATURE_PWM_HAS_INPUT_FILTER_CAPTURE */

/* A CVAL read pops one entry, so drain the pair this channel uses. A sample left
 * from an earlier run would pair with the wrong edge.
 */
static void mcux_pwm_capture_flush(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_config *config = dev->config;
	uint32_t i;

	for (i = 0; i < CAPTURE_FIFO_DEPTH; i++) {
		(void)FLEXPWM_GetInputCaptureValue(config->base, config->index,
						   mcux_pwm_capture_index(channel, false));
		(void)FLEXPWM_GetInputCaptureValue(config->base, config->index,
						   mcux_pwm_capture_index(channel, true));
	}
}

static int mcux_pwm_configure_capture(const struct device *dev, uint32_t channel,
				      pwm_flags_t flags, pwm_capture_callback_handler_t cb,
				      void *user_data)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	bool inverted = (flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED;
	flexpwm_input_capture_config_t capture_config;
	flexpwm_capture_channel_config_t *channel_config;
	int ret;

	ret = check_channel(dev, channel);
	if (ret != 0) {
		return ret;
	}

	if (cb == NULL) {
		LOG_ERR("PWM capture callback is not configured");
		return -EINVAL;
	}

	if (data->capture_active) {
		LOG_ERR("PWM capture already in progress");
		return -EBUSY;
	}

	if ((flags & PWM_CAPTURE_TYPE_MASK) == 0U) {
		LOG_ERR("No capture type specified");
		return -EINVAL;
	}

	if ((flags & PWM_CAPTURE_TYPE_MASK) == PWM_CAPTURE_TYPE_BOTH) {
		LOG_ERR("Cannot capture both period and pulse width");
		return -ENOTSUP;
	}

	data->capture.callback = cb;
	data->capture.user_data = user_data;
	data->capture.capture_channel = channel;
	data->capture.continuous =
		(flags & PWM_CAPTURE_MODE_MASK) == PWM_CAPTURE_MODE_CONTINUOUS;
	data->capture.pulse_capture =
		(flags & PWM_CAPTURE_TYPE_MASK) == PWM_CAPTURE_TYPE_PULSE;

	FLEXPWM_GetDefaultInputCaptureConfig(&capture_config);
	channel_config = mcux_pwm_capture_channel_config(&capture_config, channel);

	/* A period spans two like edges, a pulse width two opposite ones. */
	channel_config->edge0 = inverted ? kFLEXPWM_CaptureEdge_Falling
					 : kFLEXPWM_CaptureEdge_Rising;
	if (data->capture.pulse_capture) {
		channel_config->edge1 = inverted ? kFLEXPWM_CaptureEdge_Rising
						 : kFLEXPWM_CaptureEdge_Falling;
	} else {
		channel_config->edge1 = channel_config->edge0;
	}
	channel_config->oneshot = !data->capture.continuous;
	channel_config->inputSelect = kFLEXPWM_CaptureInput_RawSignal;
	channel_config->enableEdgeCounter = false;
	channel_config->edgeCompareValue = 0U;
	channel_config->fifoWatermark = kFLEXPWM_CaptureFifoWatermark_1;

	/* A submodule with no period of its own has nothing to measure against. */
	if (data->period_cycles == 0U) {
		mcux_pwm_config_capture_counter(dev);
	}

	/* FLEXPWM_ConfigInputCapture() does not touch OUTEN, and the pin has to stop
	 * driving before anything can come in on it.
	 */
	mcux_pwm_output_channel(dev, channel, false);

	FLEXPWM_ConfigInputCapture(config->base, config->index, &capture_config);

#if defined(FSL_FEATURE_PWM_HAS_INPUT_FILTER_CAPTURE) && \
	(FSL_FEATURE_PWM_HAS_INPUT_FILTER_CAPTURE == 1U)
	mcux_pwm_config_capture_filter(dev, channel);
#endif

	return 0;
}

static int mcux_pwm_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	uint16_t status;
	int ret;

	ret = check_channel(dev, channel);
	if (ret != 0) {
		return ret;
	}

	if (!data->capture.callback) {
		LOG_ERR("PWM capture not configured");
		return -EINVAL;
	}

	if (data->capture_active) {
		LOG_ERR("PWM capture already enabled");
		return -EBUSY;
	}

	data->capture_active = true;
	data->capture.reload_count = 0U;
	data->capture.modulo = (uint32_t)(config->base->SM[config->index].VAL1 -
					  config->base->SM[config->index].INIT) + 1U;

	/* The suspend hook halts the counter, which would leave an unknown gap
	 * between the two captured edges and report a bogus result with err == 0.
	 * This only keeps the system awake with CONFIG_PM_NEED_ALL_DEVICES_IDLE=y;
	 * on its own it just makes pm_suspend_devices() skip this device.
	 */
	pm_device_busy_set(dev);

	/* Keep captures off until the FIFO is empty. A stale edge would pair with a
	 * new one and get reported as a real measurement.
	 */
	FLEXPWM_DisableInputCapture(config->base, config->index,
				    mcux_pwm_capture_channel(channel));

	mcux_pwm_capture_flush(dev, channel);

	/* Clear the flags first, or the ISR can fire the moment interrupts go on and
	 * report an error for the first capture.
	 */
	status = FLEXPWM_GetSubmoduleStatusFlags(config->base, config->index);
	FLEXPWM_ClearSubmoduleStatusFlags(config->base, config->index, status);

	FLEXPWM_EnableSubmoduleInterrupts(config->base, config->index,
					  mcux_pwm_capture_irq_mask(channel));
	FLEXPWM_EnableInputCapture(config->base, config->index,
				   mcux_pwm_capture_channel(channel));

	FLEXPWM_EnableSubmoduleCounter(config->base, mcux_pwm_submodule_mask(dev));

	return 0;
}

static int mcux_pwm_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	int ret;

	ret = check_channel(dev, channel);
	if (ret != 0) {
		return ret;
	}

	mcux_pwm_capture_irq_disable(dev, channel);
	FLEXPWM_DisableInputCapture(config->base, config->index,
				    mcux_pwm_capture_channel(channel));

	data->capture_active = false;
	data->capture.callback = NULL;
	pm_device_busy_clear(dev);

	return 0;
}

/* Drop the capture on power down. There is no telling what the input did while the
 * counter was stopped, and the capture control registers may have gone back to
 * their reset values. Re-arming would time the wrong pulse, or restart a one-shot
 * that already fired, so let the caller set it up again instead. The callback runs
 * in interrupt context, so there is no way to report this from here.
 */
static void mcux_pwm_capture_invalidate(const struct device *dev)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;

	if (data->capture.callback == NULL) {
		return;
	}

	/* Disable the interrupts first, mcux_pwm_isr() calls the callback without a
	 * NULL check.
	 */
	mcux_pwm_capture_irq_disable(dev, data->capture.capture_channel);
	FLEXPWM_DisableInputCapture(config->base, config->index,
				    mcux_pwm_capture_channel(data->capture.capture_channel));

	data->capture_active = false;
	data->capture.callback = NULL;
	pm_device_busy_clear(dev);
}
#endif /* CONFIG_PWM_CAPTURE */

static int pwm_mcux_init_common(const struct device *dev)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	uint16_t submodule = mcux_pwm_submodule_mask(dev);
	flexpwm_fault_submodule_config_t fault_config;
	status_t status;
	uint32_t channel;
	int err;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &data->clock_freq)) {
		LOG_ERR("Could not get clock frequency");
		return -EINVAL;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	LOG_DBG("Set prescaler %d, reload mode %d", 1 << config->prescale, config->reload);

	status = FLEXPWM_Init(config->base);
	if (status != kStatus_Success) {
		LOG_ERR("Unable to init FlexPWM");
		return -EIO;
	}

	/* Put the submodule in a known state during initialization. */
	FLEXPWM_DisableSubmoduleInterrupts(config->base, config->index, UINT16_MAX);
	FLEXPWM_DisablePWMOutput(config->base, submodule, submodule, submodule);
	FLEXPWM_UnMaskPWMOutput(config->base, submodule, submodule, submodule);
	mcux_pwm_commit_mask(dev);

	for (channel = 0; channel < CHANNEL_COUNT; channel++) {
		data->channel[channel].masked = false;
	}

	/* No fault input may disable this submodule's outputs. */
	FLEXPWM_GetDefaultFaultSubmoduleConfig(&fault_config);
	fault_config.pwma.disableMask_ch0 = 0U;
	fault_config.pwmb.disableMask_ch0 = 0U;
	fault_config.pwmx.disableMask_ch0 = 0U;
#if defined(FSL_FEATURE_PWM_FAULT_CH_COUNT) && (FSL_FEATURE_PWM_FAULT_CH_COUNT > 1)
	fault_config.pwma.disableMask_ch1 = 0U;
	fault_config.pwmb.disableMask_ch1 = 0U;
	fault_config.pwmx.disableMask_ch1 = 0U;
#endif
	fault_config.pwma.outputBehavior = kFLEXPWM_FaultOutput_Force0;
	fault_config.pwmb.outputBehavior = kFLEXPWM_FaultOutput_Force0;
	fault_config.pwmx.outputBehavior = kFLEXPWM_FaultOutput_Force0;
	FLEXPWM_ConfigFaultSubmodule(config->base, config->index, &fault_config);

#ifdef CONFIG_PWM_CAPTURE
	if (config->irq_config_func) {
		config->irq_config_func(dev);
	}
#endif /* CONFIG_PWM_CAPTURE */

	return 0;
}

#ifdef CONFIG_PM_DEVICE
/*
 * The submodule keeps its registers but stops counting in every low power mode, so
 * hold the outputs at their inactive level before halting the counter. Otherwise the
 * pins keep whatever level they had when the clock went away. A masked pin follows
 * POLx (see above), so the requested polarity goes in here even for a channel that
 * normally runs inverted at full width.
 */
static void mcux_pwm_hold_inactive(const struct device *dev)
{
	struct pwm_mcux_data *data = dev->data;
	uint32_t channel;

	mcux_pwm_config_channels(dev, MCUX_PWM_OUTPUT_INACTIVE);

	for (channel = 0; channel < CHANNEL_COUNT; channel++) {
		if (data->channel[channel].configured) {
			mcux_pwm_mask_channel(dev, channel, true);
		}
	}
}

static void mcux_pwm_resume(const struct device *dev)
{
	struct pwm_mcux_data *data = dev->data;

	if (mcux_pwm_any_channel_configured(data)) {
		/* Restores polarity, mask, output enable and the counter in one pass. */
		mcux_pwm_program(dev, false);
		return;
	}

#ifdef CONFIG_PWM_CAPTURE
	if (data->capture_active) {
		const struct pwm_mcux_config *config = dev->config;

		FLEXPWM_EnableSubmoduleCounter(config->base, mcux_pwm_submodule_mask(dev));
	}
#endif
}
#endif /* CONFIG_PM_DEVICE */

static int mcux_pwm_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct pwm_mcux_config *config = dev->config;
	int err;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
		if (err < 0 && err != -ENOENT) {
			return err;
		}
#ifdef CONFIG_PM_DEVICE
		mcux_pwm_resume(dev);
#endif
		break;

	case PM_DEVICE_ACTION_SUSPEND:
#ifdef CONFIG_PM_DEVICE
		mcux_pwm_hold_inactive(dev);
		FLEXPWM_DisableSubmoduleCounter(config->base, mcux_pwm_submodule_mask(dev));
#endif
		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_SLEEP);
		if (err < 0 && err != -ENOENT) {
			return err;
		}
		break;

	case PM_DEVICE_ACTION_TURN_OFF:
#ifdef CONFIG_PWM_CAPTURE
		/* Clear it here, not on the way back up. A device that is switched off
		 * should not hold the busy flag and keep the system out of low power.
		 */
		mcux_pwm_capture_invalidate(dev);
#endif /* CONFIG_PWM_CAPTURE */
		break;

	case PM_DEVICE_ACTION_TURN_ON:
		err = pwm_mcux_init_common(dev);
		if (err < 0) {
			return err;
		}
#ifdef CONFIG_PM_DEVICE
		if (mcux_pwm_any_channel_configured(dev->data)) {
			/* The submodule lost its configuration, so write all of it again. */
			mcux_pwm_program(dev, true);
		}
#endif /* CONFIG_PM_DEVICE */
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int pwm_mcux_init(const struct device *dev)
{
	struct pwm_mcux_data *data = dev->data;

	k_mutex_init(&data->lock);

	/* Rest of the init is done from the PM_DEVICE_ACTION_TURN_ON action
	 * which is invoked by pm_device_driver_init().
	 */
	return pm_device_driver_init(dev, mcux_pwm_pm_action);
}

static DEVICE_API(pwm, pwm_mcux_driver_api) = {
	.set_cycles = mcux_pwm_set_cycles,
	.get_cycles_per_sec = mcux_pwm_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = mcux_pwm_configure_capture,
	.enable_capture = mcux_pwm_enable_capture,
	.disable_capture = mcux_pwm_disable_capture,
#endif
};

#ifdef CONFIG_PWM_CAPTURE

#define PWM_MCUX_IRQ_CONFIG_FUNC(n) \
	static void pwm_mcux_config_func_##n(const struct device *dev) \
	{	\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), \
				mcux_pwm_isr, DEVICE_DT_INST_GET(n), 0); \
		irq_enable(DT_INST_IRQN(n));	\
	}
#define PWM_MCUX_CAPTURE_CONFIG_INIT(n) \
	.irq_config_func = pwm_mcux_config_func_##n,	\
	.input_filter_count = DT_INST_PROP_OR(n, input_filter_count, 0),	\
	.input_filter_period = DT_INST_PROP_OR(n, input_filter_period, 0),
#else
#define PWM_MCUX_IRQ_CONFIG_FUNC(n)
#define PWM_MCUX_CAPTURE_CONFIG_INIT(n)
#endif /* CONFIG_PWM_CAPTURE */

#define PWM_DEVICE_INIT_MCUX(n)			  \
	static struct pwm_mcux_data pwm_mcux_data_ ## n;		  \
	PINCTRL_DT_INST_DEFINE(n);					  \
	PWM_MCUX_IRQ_CONFIG_FUNC(n)					  \
									  \
	static const struct pwm_mcux_config pwm_mcux_config_ ## n = {     \
		.base = (PWM_Type *)DT_REG_ADDR(DT_INST_PARENT(n)),	  \
		.index = DT_INST_PROP(n, index),			  \
		.prescale = _CONCAT(kFLEXPWM_Prescale_Divide_,		  \
				    DT_INST_PROP(n, nxp_prescaler)),	  \
		.reload = DT_ENUM_IDX_OR(DT_DRV_INST(n), nxp_reload,	  \
					 RELOAD_FULL_CYCLE),		  \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),\
		.run_wait = DT_INST_PROP(n, run_in_wait),		  \
		.run_debug = DT_INST_PROP(n, run_in_debug),		  \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		  \
		PWM_MCUX_CAPTURE_CONFIG_INIT(n)	\
	};								  \
									  \
	PM_DEVICE_DT_INST_DEFINE(n, mcux_pwm_pm_action);			  \
									  \
	DEVICE_DT_INST_DEFINE(n,					  \
			    pwm_mcux_init,				  \
			    PM_DEVICE_DT_INST_GET(n),			  \
			    &pwm_mcux_data_ ## n,			  \
			    &pwm_mcux_config_ ## n,			  \
			    POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,	  \
			    &pwm_mcux_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT_MCUX)
