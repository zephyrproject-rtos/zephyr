/*
 * Copyright (c) 2019, Linaro
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_pwm

#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/clock_control.h>
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
#ifdef CONFIG_PWM_CAPTURE
	struct pwm_mcux_capture_data capture;
	bool capture_active;
#endif
};

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

static void mcux_pwm_isr(const struct device *dev)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	struct pwm_mcux_capture_data *capture = &data->capture;
	uint32_t status;
	uint16_t first_edge_value;
	uint16_t second_edge_value;
	uint32_t ticks = 0;
	int err = 0;

	uint16_t modValue = config->base->SM[config->index].VAL1 -
		config->base->SM[config->index].INIT;

	status = PWM_GetStatusFlags(config->base, config->index);
	PWM_ClearStatusFlags(config->base, config->index, status);

	if (status & kPWM_ReloadFlag) {
		err = u32_add_overflow(capture->overflow_count, 1, &capture->overflow_count);
	}

	if (status & kPWM_CaptureX0Flag) {
		capture->overflow_count = 0;
	}

	if (status & kPWM_CaptureX1Flag) {
		if (err != 0) {
			LOG_ERR("overflow_count overflows.");
		} else {
			first_edge_value = config->base->SM[config->index].CVAL0;
			second_edge_value = config->base->SM[config->index].CVAL1;
			err = mcux_pwm_calc_ticks(first_edge_value, second_edge_value, modValue,
					capture->overflow_count, &ticks);
			LOG_DBG("First edge capture: %d, second edge capture: %d,"
				"overflow: %d, ticks: %d", first_edge_value, second_edge_value,
				capture->overflow_count, ticks);
		}

		if (capture->pulse_capture) {
			capture->callback(dev, capture->capture_channel, 0, ticks, err,
					capture->user_data);
		} else {
			capture->callback(dev, capture->capture_channel, ticks, 0, err,
					capture->user_data);
		}

		capture->overflow_count = 0;
	}
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

	/* Setup input capture on channel */
	PWM_SetupInputCapture(config->base, config->index, pwm_channel, &capture_config);

	/* Set capture filter */
	PWM_SetFilterSampleCount(config->base, pwm_channel, config->index,
		config->input_filter_count);
	PWM_SetFilterSamplePeriod(config->base, pwm_channel, config->index,
		config->input_filter_period);

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
	/* Make sure the flags are cleared in case it enters IRQ immediately after enable
	 * interrupts, results in error result at first.
	 */
	status = PWM_GetStatusFlags(config->base, config->index);
	PWM_ClearStatusFlags(config->base, config->index, status);

	if (channel == 0U) {
		PWM_EnableInterrupts(config->base, config->index, kPWM_CaptureA0InterruptEnable |
			kPWM_CaptureA1InterruptEnable | kPWM_ReloadInterruptEnable);
	} else if (channel == 1U) {
		PWM_EnableInterrupts(config->base, config->index, kPWM_CaptureB0InterruptEnable |
			kPWM_CaptureB1InterruptEnable | kPWM_ReloadInterruptEnable);
	} else {
		PWM_EnableInterrupts(config->base, config->index, kPWM_CaptureX0InterruptEnable |
			kPWM_CaptureX1InterruptEnable | kPWM_ReloadInterruptEnable);
	}

	/* Start the PWM counter if it's stopped.*/
	if ((config->base->MCTRL & PWM_MCTRL_RUN_MASK) == 0) {
		PWM_StartTimer(config->base, (1U << config->index));
	}

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

	/* Disable capture interrupts */
	if (channel == 0U) {
		PWM_DisableInterrupts(config->base, config->index, kPWM_CaptureA0InterruptEnable |
			kPWM_CaptureA1InterruptEnable | kPWM_ReloadInterruptEnable);
	} else if (channel == 1U) {
		PWM_DisableInterrupts(config->base, config->index, kPWM_CaptureB0InterruptEnable |
			kPWM_CaptureB1InterruptEnable | kPWM_ReloadInterruptEnable);
	} else {
		PWM_DisableInterrupts(config->base, config->index, kPWM_CaptureX0InterruptEnable |
			kPWM_CaptureX1InterruptEnable | kPWM_ReloadInterruptEnable);
	}

	data->capture_active = false;
	data->capture.callback = NULL;

	return 0;
}
#endif /* CONFIG_PWM_CAPTURE */

static int pwm_mcux_init(const struct device *dev)
{
	const struct pwm_mcux_config *config = dev->config;
	struct pwm_mcux_data *data = dev->data;
	pwm_config_t pwm_config;
	status_t status;
	int i, err;

	k_mutex_init(&data->lock);

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

#ifdef CONFIG_PWM_CAPTURE
	if (config->irq_config_func) {
		config->irq_config_func(dev);
	}
#endif

	return 0;
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
	DEVICE_DT_INST_DEFINE(n,					  \
			    pwm_mcux_init,				  \
			    NULL,					  \
			    &pwm_mcux_data_ ## n,			  \
			    &pwm_mcux_config_ ## n,			  \
			    POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,	  \
			    &pwm_mcux_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT_MCUX)
