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
#include <fsl_pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_PWM_CAPTURE
#include <zephyr/irq.h>
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_mcux, CONFIG_PWM_LOG_LEVEL);

#define CHANNEL_COUNT 3

#ifdef CONFIG_PM_DEVICE
/* Channel setup replayed after the submodule is powered back on. */
struct pwm_mcux_channel_config {
	uint32_t period_cycles;
	uint32_t pulse_cycles;
	pwm_flags_t flags;
};
#endif /* CONFIG_PM_DEVICE */

#ifdef CONFIG_PWM_CAPTURE
struct pwm_mcux_capture_data {
	pwm_capture_callback_handler_t callback;
	void *user_data;
	uint32_t overflow_count;
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
	pwm_clock_prescale_t prescale;
	pwm_register_reload_t reload;
	pwm_mode_t mode;
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
	uint32_t period_cycles[CHANNEL_COUNT];
	uint32_t pulse_cycles[CHANNEL_COUNT];
	pwm_signal_param_t channel[CHANNEL_COUNT];
	struct k_mutex lock;
#ifdef CONFIG_PM_DEVICE
	struct pwm_mcux_channel_config channel_config[CHANNEL_COUNT];
	bool pwm_channel_active;
#endif /* CONFIG_PM_DEVICE */
#ifdef CONFIG_PWM_CAPTURE
	struct pwm_mcux_capture_data capture;
	bool capture_active;
#endif
};

/*
 * Mask a channel to its inactive level, or hand the pin back to the generator.
 *
 * The mask forces the channel signal to 0 ahead of the polarity stage, so a
 * masked pin settles on OCTRL POLx. That is already the inactive level for
 * channels A and B, but channel X runs with POLX inverted (its pulse width comes
 * from VAL0 alone), so POLX has to be flipped while masked.
 */
static void mcux_pwm_force_inactive(const struct device *dev, uint32_t channel, bool force)
{
	static const pwm_channels_t pwm_channel[CHANNEL_COUNT] = {
		kPWM_PwmA,
		kPWM_PwmB,
		kPWM_PwmX,
	};
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	bool polx;

	if (channel == 2) {
		if (force) {
			polx = (data->channel[channel].level == kPWM_LowTrue);
		} else {
			polx = (data->channel[channel].level == kPWM_HighTrue);
		}

		if (polx) {
			config->base->SM[config->index].OCTRL |=
				((uint16_t)1U << PWM_OCTRL_POLX_SHIFT);
		} else {
			config->base->SM[config->index].OCTRL &=
				~((uint16_t)1U << PWM_OCTRL_POLX_SHIFT);
		}
	}

	PWM_SetPwmForceOutputToZero(config->base, config->index, pwm_channel[channel], force);
}

static int mcux_pwm_set_cycles_internal(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	pwm_level_select_t level;
	uint32_t pwm_clk_freq;

	pwm_clk_freq = data->clock_freq >> config->prescale;

#ifdef CONFIG_PWM_CAPTURE
	if (data->capture_active) {
		LOG_ERR("PWM capture is active, cannot set PWM output");
		return -EBUSY;
	}
#endif

	if (flags & PWM_POLARITY_INVERTED) {
		level = kPWM_LowTrue;
	} else {
		level = kPWM_HighTrue;
	}

	if (period_cycles != data->period_cycles[channel]
	    || level != data->channel[channel].level) {
		status_t status;

		data->period_cycles[channel] = period_cycles;

		data->channel[channel].pwmchannelenable = true;

		PWM_StopTimer(config->base, 1U << config->index);

		/*
		 * We will directly write the duty cycle pulse width
		 * and full pulse width into the VALx registers to
		 * setup PWM with higher resolution.
		 * Therefore we use dummy values for the duty cycle
		 * and frequency.
		 */
		data->channel[channel].dutyCyclePercent = 0;
		data->channel[channel].level = level;
		data->pulse_cycles[channel] = pulse_cycles;

		status = PWM_SetupPwm(config->base, config->index,
				      &data->channel[channel], 1U,
				      config->mode, pwm_clk_freq, data->clock_freq);
		if (status != kStatus_Success) {
			LOG_ERR("Could not set up pwm (%d)", status);
			return -ENOTSUP;
		}

		if (channel == 2) {
			/* For channels A/B, when the counter matches VAL2/VAL4 or VAL3/VAL5,
			 * the output status changes. VAL2 and VAL4 are set to 0, so the channel
			 * output is high at the beginning of the period, then becomes low when
			 * the counter matches VAL3/VAL5 (pulse width).
			 * Channel X only uses VAL0 for pulse width, so its polarity must be
			 * handled differently.
			 */
			if (level == kPWM_HighTrue) {
				config->base->SM[config->index].OCTRL |=
					((uint16_t)1U << PWM_OCTRL_POLX_SHIFT);
			} else {
				config->base->SM[config->index].OCTRL &=
					~((uint16_t)1U << PWM_OCTRL_POLX_SHIFT);
			}
		} else if (data->period_cycles[2] != 0U) {
			/* When setting channel A/B, PWM_SetupPwm internally calls
			 * PWM_SetDutycycleRegister which modifies VAL0. Since VAL0 controls
			 * channel X's pulse width, we need to restore it to maintain channel X's
			 * configured pulse cycles.
			 */
			config->base->SM[config->index].VAL0 = data->pulse_cycles[2];
		} else {
			/* No action required. */
		}

		/* Setup VALx values directly for edge aligned PWM */
		if (channel == 0) {
			/* Side A */
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_1,
					 (uint16_t)(period_cycles - 1U));
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_2, 0U);
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_3,
					 (uint16_t)pulse_cycles);
		} else if (channel == 1) {
			/* Side B */
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_1,
					 (uint16_t)(period_cycles - 1U));
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_4, 0U);
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_5,
					 (uint16_t)pulse_cycles);
		} else {
			/* Side X */
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_0,
					 (uint16_t)pulse_cycles);
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_1,
					 (uint16_t)(period_cycles - 1U));
		}

		PWM_SetPwmLdok(config->base, 1U << config->index, true);

		PWM_StartTimer(config->base, 1U << config->index);
	} else {
		uint64_t period_time_us =
			(uint64_t)data->period_cycles[channel] * 1000000U / (pwm_clk_freq);
		__ASSERT_NO_MSG(period_time_us <= 0xFFFFFFFFU);
		/* Wait for the registers to finish their previous load (LDOK cleared).
		 * The LDOK is cleared after one PWM period, so we wait period_time_us.
		 * Keep 1 millisecond here for compatibility.
		 */
		bool ldok_got_cleared = WAIT_FOR(
			!(config->base->MCTRL & PWM_MCTRL_LDOK(1U << config->index)),
			MAX(1000, (uint32_t)period_time_us),
			k_busy_wait(1) /* busywait meanwhile */
		);

		if (!ldok_got_cleared) {
			/*
			 * LDOK didn't get cleared in timeout, which is extremely rare.
			 * We return with an error though, because setting the VALx values in
			 * this state would do nothing
			 */
			return -EBUSY;
		}

		/* Setup VALx values directly for edge aligned PWM */
		if (channel == 0) {
			/* Side A */
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_2, 0U);
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_3,
					 (uint16_t)pulse_cycles);
		} else if (channel == 1) {
			/* Side B */
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_4, 0U);
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_5,
					 (uint16_t)pulse_cycles);
		} else {
			/* Side X */
			PWM_SetVALxValue(config->base, config->index,
					 kPWM_ValueRegister_0,
					 (uint16_t)pulse_cycles);
		}
		PWM_SetPwmLdok(config->base, 1U << config->index, true);
	}

	/* A zero pulse width must hold the inactive level, but on channel X a VAL0
	 * of 0 still lets a one tick pulse through at the reload boundary.
	 */
	mcux_pwm_force_inactive(dev, channel, pulse_cycles == 0U);

	return 0;
}

static int mcux_pwm_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	struct pwm_mcux_data *data = dev->data;
	int result;

	if (channel >= CHANNEL_COUNT) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (period_cycles == 0) {
		LOG_ERR("Channel can not be set to inactive level");
		return -ENOTSUP;
	}

	if (period_cycles > UINT16_MAX) {
		/* 16-bit resolution */
		LOG_ERR("Too long period (%u), adjust pwm prescaler!",
			period_cycles);
		/* TODO: dynamically adjust prescaler */
		return -EINVAL;
	}

#ifdef CONFIG_PM_DEVICE
	/* Remember the setup for replay after a power down. */
	data->channel_config[channel].period_cycles = period_cycles;
	data->channel_config[channel].pulse_cycles = pulse_cycles;
	data->channel_config[channel].flags = flags;
	data->pwm_channel_active = true;
#endif /* CONFIG_PM_DEVICE */

	k_mutex_lock(&data->lock, K_FOREVER);
	result = mcux_pwm_set_cycles_internal(dev, channel, period_cycles, pulse_cycles, flags);
	k_mutex_unlock(&data->lock);
	return result;
}

static int mcux_pwm_get_cycles_per_sec(const struct device *dev,
				       uint32_t channel, uint64_t *cycles)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;

	*cycles = data->clock_freq >> config->prescale;

	return 0;
}

#ifdef CONFIG_PWM_CAPTURE
static int mcux_pwm_calc_ticks(uint16_t first_capture, uint16_t second_capture, uint32_t mod,
			       uint32_t overflows, uint32_t *result)
{
	uint32_t ticks;

	if (second_capture >= first_capture) {
		/* No timer overflow between captures */
		ticks = second_capture - first_capture;
	} else {
		/* Timer overflowed between captures */
		ticks = (mod - first_capture) + second_capture + 1U;
		if (overflows > 0) {
			/* Account for the overflow we just calculated */
			overflows--;
		}
	}

	/* Add additional overflows */
	if (u32_mul_overflow(overflows, mod, &overflows)) {
		LOG_ERR("overflow while calculating overflow cycles.");
		return -ERANGE;
	}

	if (u32_add_overflow(ticks, overflows, &ticks)) {
		LOG_ERR("overflow while calculating capture cycles.");
		return -ERANGE;
	}

	*result = ticks;

	return 0;
}

static void mcux_pwm_capture_irq_disable(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_config *config = dev->config;

	switch (channel) {
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELA) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELA == 1U)
	case 0U:
		/* Channel A */
		PWM_DisableInterrupts(config->base, config->index, kPWM_CaptureA0InterruptEnable |
			kPWM_CaptureA1InterruptEnable | kPWM_ReloadInterruptEnable);
		break;
#endif
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELB) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELB == 1U)
	case 1U:
		/* Channel B */
		PWM_DisableInterrupts(config->base, config->index, kPWM_CaptureB0InterruptEnable |
			kPWM_CaptureB1InterruptEnable | kPWM_ReloadInterruptEnable);
		break;
#endif
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELX) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELX == 1U)
	case 2U:
		/* Channel X */
		PWM_DisableInterrupts(config->base, config->index, kPWM_CaptureX0InterruptEnable |
			kPWM_CaptureX1InterruptEnable | kPWM_ReloadInterruptEnable);
		break;
#endif
	default:
		/* unsupported channel for this SoC: check_channel() should have rejected it */
		break;
	}
}

static void mcux_pwm_handle_capture(const struct device *dev, uint16_t first_edge_value,
				    uint16_t second_edge_value, uint16_t modValue,
				    int overflow_err)
{
	struct pwm_mcux_data *data = dev->data;
	struct pwm_mcux_capture_data *capture = &data->capture;
	uint32_t ticks = 0;
	int err = overflow_err;

	if (err != 0) {
		LOG_ERR("overflow_count overflows.");
	} else {
		err = mcux_pwm_calc_ticks(first_edge_value, second_edge_value, modValue,
				capture->overflow_count, &ticks);
		LOG_DBG("First edge capture: %u, second edge capture: %u,"
			" overflow: %u, ticks: %u", first_edge_value,
			second_edge_value, capture->overflow_count, ticks);
	}

	if (capture->pulse_capture) {
		capture->callback(dev, capture->capture_channel, 0, ticks, err,
				capture->user_data);
	} else {
		capture->callback(dev, capture->capture_channel, ticks, 0, err,
				capture->user_data);
	}

	capture->overflow_count = 0;

	if (!capture->continuous) {
		/* One shot capture is done: the hardware stops arming, so drop the
		 * interrupts that would keep firing on every reload. The channel
		 * stays claimed until pwm_disable_capture(), as the API expects.
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
	uint32_t status;
	uint16_t first_edge_value;
	uint16_t second_edge_value;
	int err = 0;

	uint16_t modValue = config->base->SM[config->index].VAL1 -
		config->base->SM[config->index].INIT;

	status = PWM_GetStatusFlags(config->base, config->index);
	PWM_ClearStatusFlags(config->base, config->index, status);

	if (status & kPWM_ReloadFlag) {
		err = u32_add_overflow(capture->overflow_count, 1, &capture->overflow_count);
	}

	switch (capture->capture_channel) {
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELA) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELA == 1U)
	case 0:
		/* Channel A */
		if (status & kPWM_CaptureA0Flag) {
			capture->overflow_count = 0;
		}

		if (status & kPWM_CaptureA1Flag) {
			first_edge_value = config->base->SM[config->index].CVAL2;
			second_edge_value = config->base->SM[config->index].CVAL3;
			LOG_DBG("Channel A captured.");
			mcux_pwm_handle_capture(dev, first_edge_value, second_edge_value,
				modValue, err);
		}
		break;
#endif
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELB) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELB == 1U)
	case 1:
		/* Channel B */
		if (status & kPWM_CaptureB0Flag) {
			capture->overflow_count = 0;
		}

		if (status & kPWM_CaptureB1Flag) {
			first_edge_value = config->base->SM[config->index].CVAL4;
			second_edge_value = config->base->SM[config->index].CVAL5;
			LOG_DBG("Channel B captured.");
			mcux_pwm_handle_capture(dev, first_edge_value, second_edge_value,
						modValue, err);
		}
		break;
#endif
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELX) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELX == 1U)
	case 2:
		/* Channel X */
		if (status & kPWM_CaptureX0Flag) {
			capture->overflow_count = 0;
		}

		if (status & kPWM_CaptureX1Flag) {
			first_edge_value = config->base->SM[config->index].CVAL0;
			second_edge_value = config->base->SM[config->index].CVAL1;
			LOG_DBG("Channel X captured.");
			mcux_pwm_handle_capture(dev, first_edge_value, second_edge_value,
						modValue, err);
		}
		break;
#endif
	default:
		/* unsupported channel for this SoC: check_channel() should have rejected it */
		break;
	}
}

/* Free running modulo for a capture-only submodule. The counter is signed
 * (reference manual, "Counter Register"), so 0x7FFF is the maximum with INIT = 0.
 */
#define CAPTURE_ONLY_MODULO 0x7FFFU

static bool mcux_pwm_period_configured(const struct pwm_mcux_data *data)
{
	uint32_t channel;

	for (channel = 0; channel < CHANNEL_COUNT; channel++) {
		if (data->period_cycles[channel] != 0U) {
			return true;
		}
	}

	return false;
}

/* mcux_pwm_isr() derives the overflow size from VAL1 and INIT, which only
 * mcux_pwm_set_cycles() ever programs. A capture-only submodule would keep their
 * reset value of 0, making the reported tick count meaningless.
 */
static void mcux_pwm_setup_capture_modulo(const struct device *dev)
{
	const struct pwm_mcux_config *config = dev->config;
	uint16_t ctrl;

	config->base->SM[config->index].INIT = 0U;
	PWM_SetVALxValue(config->base, config->index, kPWM_ValueRegister_1,
			 CAPTURE_ONLY_MODULO);

	/* INIT and VAL1 are buffered and would only load on the next reload, which
	 * never comes while the counter is stopped, so force it through LDMOD.
	 */
	ctrl = config->base->SM[config->index].CTRL;
	config->base->SM[config->index].CTRL = ctrl | PWM_CTRL_LDMOD_MASK;
	PWM_SetPwmLdok(config->base, 1U << config->index, true);
	config->base->SM[config->index].CTRL = ctrl;
}

static int check_channel(const struct device *dev, uint32_t channel)
{
	struct pwm_mcux_data *data = dev->data;

	/* Check if the channel is already used for PWM output */
	if (channel < CHANNEL_COUNT && data->period_cycles[channel] != 0) {
		LOG_ERR("Channel %d is already used for PWM output", channel);
		return -EBUSY;
	}

	/* Check if the channel supports capture based on hardware features */
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
	} else if (channel == 2U) {
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELX) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELX == 0U)
		LOG_ERR("Channel X does not support capture on this hardware");
		return -ENOTSUP;
#endif
	} else {
		LOG_ERR("Invalid channel %d", channel);
		return -EINVAL;
	}

	return 0;
}

static int mcux_pwm_configure_capture(const struct device *dev,
				      uint32_t channel, pwm_flags_t flags,
				      pwm_capture_callback_handler_t cb,
				      void *user_data)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	bool inverted = (flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED;
	int ret;
	pwm_channels_t pwm_channel;
	pwm_input_capture_param_t capture_config;

	memset(&capture_config, 0, sizeof(capture_config));

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

	if (!(flags & PWM_CAPTURE_TYPE_MASK)) {
		LOG_ERR("No capture type specified");
		return -EINVAL;
	}

	if ((flags & PWM_CAPTURE_TYPE_MASK) == PWM_CAPTURE_TYPE_BOTH) {
		LOG_ERR("Cannot capture both period and pulse width");
		return -ENOTSUP;
	}

	/* Initialize capture data */
	data->capture.callback = cb;
	data->capture.user_data = user_data;
	data->capture.capture_channel = channel;
	data->capture.continuous =
		(flags & PWM_CAPTURE_MODE_MASK) == PWM_CAPTURE_MODE_CONTINUOUS;
	data->capture.pulse_capture =
		(flags & PWM_CAPTURE_TYPE_MASK) == PWM_CAPTURE_TYPE_PULSE;
	data->capture.overflow_count = 0;

	/* Configure input capture parameters */
	capture_config.captureInputSel = false;        /* Use raw input signal (not edge counter) */
	if (data->capture.pulse_capture) {
		capture_config.edge0 = inverted ? kPWM_FallingEdge : kPWM_RisingEdge;
		capture_config.edge1 = inverted ? kPWM_RisingEdge : kPWM_FallingEdge;
	} else {
		capture_config.edge0 = inverted ? kPWM_FallingEdge : kPWM_RisingEdge;
		capture_config.edge1 = inverted ? kPWM_FallingEdge : kPWM_RisingEdge;
	}
	capture_config.enableOneShotCapture = !data->capture.continuous;
	capture_config.fifoWatermark = 0;

	/* Fix mismatch because kPWM_PwmA is 1U, kPWM_PwmB is 0U */
	if (channel == 0U) {
		pwm_channel = kPWM_PwmA;
	} else if (channel == 1U) {
		pwm_channel = kPWM_PwmB;
	} else {
		pwm_channel = kPWM_PwmX;
	}

	/* A capture-only submodule has no period, so nothing set the modulo. */
	if (!mcux_pwm_period_configured(data)) {
		mcux_pwm_setup_capture_modulo(dev);
	}

	/* Setup input capture on channel */
	PWM_SetupInputCapture(config->base, config->index, pwm_channel, &capture_config);

#if defined(FSL_FEATURE_PWM_HAS_INPUT_FILTER_CAPTURE) && \
	(FSL_FEATURE_PWM_HAS_INPUT_FILTER_CAPTURE == 1U)
	/* Set capture filter */
	PWM_SetFilterSampleCount(config->base, pwm_channel, config->index,
		config->input_filter_count);
	PWM_SetFilterSamplePeriod(config->base, pwm_channel, config->index,
		config->input_filter_period);
#endif

	return 0;
}

static int mcux_pwm_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	uint32_t status;
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

	/* The suspend hook halts the counter, which would leave an unknown gap
	 * between the two captured edges and report a bogus result with err == 0.
	 * This only keeps the system awake with CONFIG_PM_NEED_ALL_DEVICES_IDLE=y;
	 * on its own it just makes pm_suspend_devices() skip this device.
	 */
	pm_device_busy_set(dev);

	/* Make sure the flags are cleared in case it enters IRQ immediately after enable
	 * interrupts, results in error result at first.
	 */
	status = PWM_GetStatusFlags(config->base, config->index);
	PWM_ClearStatusFlags(config->base, config->index, status);
	/* Enable interrupt and clear the capture FIFOs by reading them */
	switch (channel) {
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELA) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELA == 1U)
	case 0U:
		/* Channel A */
		(void)config->base->SM[config->index].CVAL2;
		(void)config->base->SM[config->index].CVAL3;
		PWM_EnableInterrupts(config->base, config->index, kPWM_CaptureA0InterruptEnable |
			kPWM_CaptureA1InterruptEnable | kPWM_ReloadInterruptEnable);
		break;
#endif
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELB) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELB == 1U)
	case 1U:
		/* Channel B */
		(void)config->base->SM[config->index].CVAL4;
		(void)config->base->SM[config->index].CVAL5;
		PWM_EnableInterrupts(config->base, config->index, kPWM_CaptureB0InterruptEnable |
			kPWM_CaptureB1InterruptEnable | kPWM_ReloadInterruptEnable);
		break;
#endif
#if defined(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELX) && \
	(FSL_FEATURE_PWM_HAS_CAPTURE_ON_CHANNELX == 1U)
	case 2U:
		/* Channel X */
		(void)config->base->SM[config->index].CVAL0;
		(void)config->base->SM[config->index].CVAL1;
		PWM_EnableInterrupts(config->base, config->index, kPWM_CaptureX0InterruptEnable |
			kPWM_CaptureX1InterruptEnable | kPWM_ReloadInterruptEnable);
		break;
#endif
	default:
		/* unsupported channel for this SoC: check_channel() should have rejected it */
		break;
	}

	/* MCTRL[RUN] has one bit per submodule: testing the whole field would let
	 * another running submodule hide that this one is halted.
	 */
	if ((config->base->MCTRL & PWM_MCTRL_RUN(1U << config->index)) == 0) {
		PWM_StartTimer(config->base, (1U << config->index));
	}

	return 0;
}

static int mcux_pwm_disable_capture(const struct device *dev, uint32_t channel)
{
	struct pwm_mcux_data *data = dev->data;
	int ret;

	ret = check_channel(dev, channel);
	if (ret != 0) {
		return ret;
	}

	/* Disable capture interrupts */
	mcux_pwm_capture_irq_disable(dev, channel);

	data->capture_active = false;
	data->capture.callback = NULL;
	pm_device_busy_clear(dev);

	return 0;
}
#endif /* CONFIG_PWM_CAPTURE */

static int pwm_mcux_init_common(const struct device *dev)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	pwm_config_t pwm_config;
	status_t status;
	int i, err;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &data->clock_freq)) {
		LOG_ERR("Could not get clock frequency");
		return -EINVAL;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	LOG_DBG("Set prescaler %d, reload mode %d",
			1 << config->prescale, config->reload);

	PWM_GetDefaultConfig(&pwm_config);
	pwm_config.prescale = config->prescale;
	pwm_config.reloadLogic = config->reload;
	pwm_config.clockSource = kPWM_BusClock;
	pwm_config.enableDebugMode = config->run_debug;
#if !defined(FSL_FEATURE_PWM_HAS_NO_WAITEN) || (!FSL_FEATURE_PWM_HAS_NO_WAITEN)
	/* Note: When the CPU enters a low-power mode, if enableWait is not set to true,
	 * the FlexPWM module will stop operating, which may interfere with input capture
	 * functionality
	 */
	pwm_config.enableWait = config->run_wait;
#endif

	status = PWM_Init(config->base, config->index, &pwm_config);
	if (status != kStatus_Success) {
		LOG_ERR("Unable to init PWM");
		return -EIO;
	}

	/* Disable fault sources */
	for (i = 0; i < FSL_FEATURE_PWM_FAULT_CH_COUNT; i++) {
		config->base->SM[config->index].DISMAP[i] = 0x0000;
	}

	data->channel[0].pwmChannel = kPWM_PwmA;
	data->channel[0].level = kPWM_HighTrue;
	data->channel[1].pwmChannel = kPWM_PwmB;
	data->channel[1].level = kPWM_HighTrue;
	data->channel[2].pwmChannel = kPWM_PwmX;
	data->channel[2].level = kPWM_HighTrue;

	/* The submodule is back in reset state, so drop the cached VALx contents and
	 * make the next set_cycles() take the full setup path.
	 */
	for (i = 0; i < CHANNEL_COUNT; i++) {
		data->period_cycles[i] = 0U;
		data->pulse_cycles[i] = 0U;
	}

#ifdef CONFIG_PWM_CAPTURE
	if (config->irq_config_func) {
		config->irq_config_func(dev);
	}
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE
/* Park every configured channel, or release them. A zero pulse width stays forced
 * either way, so releasing does not resurrect an output the API wanted held
 * inactive.
 */
static void mcux_pwm_park_channels(const struct device *dev, bool park)
{
	struct pwm_mcux_data *data = dev->data;
	uint32_t channel;

	for (channel = 0; channel < CHANNEL_COUNT; channel++) {
		if (data->period_cycles[channel] == 0U) {
			continue;
		}

		mcux_pwm_force_inactive(dev, channel,
				       park || data->pulse_cycles[channel] == 0U);
	}
}

static void mcux_pwm_restore_chn_config(const struct device *dev)
{
	struct pwm_mcux_data *data = dev->data;
	uint32_t channel;

	/* Ascending order matters: set_cycles() only restores channel X's VAL0 for
	 * channels A/B once channel X has been set up.
	 */
	for (channel = 0; channel < CHANNEL_COUNT; channel++) {
		if (data->channel_config[channel].period_cycles == 0U) {
			continue;
		}

		(void)mcux_pwm_set_cycles(dev, channel, data->channel_config[channel].period_cycles,
					  data->channel_config[channel].pulse_cycles,
					  data->channel_config[channel].flags);
	}
}

static bool mcux_pwm_timer_needed(const struct device *dev)
{
	struct pwm_mcux_data *data = dev->data;

	if (data->pwm_channel_active) {
		return true;
	}

#ifdef CONFIG_PWM_CAPTURE
	if (data->capture_active) {
		return true;
	}
#endif /* CONFIG_PWM_CAPTURE */

	return false;
}
#endif /* CONFIG_PM_DEVICE */

static int mcux_pwm_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct pwm_mcux_config *config = dev->config;
	int err;
#ifdef CONFIG_PM_DEVICE
	struct pwm_mcux_data *data = dev->data;
#endif /* CONFIG_PM_DEVICE */

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
		if (err < 0 && err != -ENOENT) {
			return err;
		}
#ifdef CONFIG_PM_DEVICE
		/* Release the parked outputs before the generator drives them. */
		mcux_pwm_park_channels(dev, false);

		if (mcux_pwm_timer_needed(dev)) {
			PWM_StartTimer(config->base, 1U << config->index);
		}
#endif /* CONFIG_PM_DEVICE */
		break;

	case PM_DEVICE_ACTION_SUSPEND:
#ifdef CONFIG_PM_DEVICE
		/* The submodule keeps its registers but stops counting in every low
		 * power mode, so park the outputs before halting the counter or the
		 * pins would hold whatever level they had when the clock went away.
		 */
		mcux_pwm_park_channels(dev, true);
		PWM_StopTimer(config->base, 1U << config->index);
#endif /* CONFIG_PM_DEVICE */
		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_SLEEP);
		if (err < 0 && err != -ENOENT) {
			return err;
		}
		break;

	case PM_DEVICE_ACTION_TURN_OFF:
		break;

	case PM_DEVICE_ACTION_TURN_ON:
		err = pwm_mcux_init_common(dev);
		if (err < 0) {
			return err;
		}
#ifdef CONFIG_PM_DEVICE
		if (data->pwm_channel_active) {
			mcux_pwm_restore_chn_config(dev);
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
		.mode = kPWM_EdgeAligned,				  \
		.prescale = _CONCAT(kPWM_Prescale_Divide_, DT_INST_PROP(n, nxp_prescaler)),\
		.reload = DT_ENUM_IDX_OR(DT_DRV_INST(n), nxp_reload,\
				kPWM_ReloadPwmFullCycle),\
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
