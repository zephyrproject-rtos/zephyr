/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT nxp_flexio_pwm

#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <fsl_flexio.h>
#include <fsl_clock.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>

#include <zephyr/logging/log.h>
#include <zephyr/drivers/misc/nxp_flexio/nxp_flexio.h>


LOG_MODULE_REGISTER(pwm_nxp_flexio, CONFIG_PWM_LOG_LEVEL);

#define FLEXIO_PWM_TIMER_CMP_MAX_VALUE		(0xFFFFU)
#define FLEXIO_PWM_TIMCMP_CMP_UPPER_SHIFT	(0x8U)
#define FLEXIO_MAX_PWM_CHANNELS			8

enum pwm_nxp_flexio_polarity {
	FLEXIO_PWM_ACTIVE_HIGH   = 0x0U,
	FLEXIO_PWM_ACTIVE_LOW    = 0x1U
};

enum pwm_nxp_flexio_timerinit {
	/** Timer Initial output is logic one */
	FLEXIO_PWM_TIMER_INIT_HIGH   = 0x00U,
	/** Timer Initial output is logic zero */
	FLEXIO_PWM_TIMER_INIT_LOW    = 0x1U
};

enum pwm_nxp_flexio_prescaler {
	/* Decrement counter on Flexio clock */
	FLEXIO_PWM_CLK_DIV_1     = 0U,
	/* Decrement counter on Flexio clock divided by 16 */
	FLEXIO_PWM_CLK_DIV_16    = 4U,
	/* Decrement counter on Flexio clock divided by 256 */
	FLEXIO_PWM_CLK_DIV_256   = 5U
};

enum pwm_nxp_flexio_timer_mode {
	/** Timer disabled */
	FLEXIO_PWM_TIMER_DISABLED  = 0x00U,
	/** Timer in 8 bit Pwm High mode */
	FLEXIO_PWM_TIMER_PWM_HIGH  = 0x02U,
	/** Timer in 8 bit Pwm Low mode */
	FLEXIO_PWM_TIMER_PWM_LOW   = 0x06U
};

enum pwm_nxp_flexio_timer_pin {
	/** Timer Pin output disabled */
	FLEXIO_PWM_TIMER_PIN_OUTPUT_DISABLE  = 0x00U,
	/** Timer Pin Output mode */
	FLEXIO_PWM_TIMER_PIN_OUTPUT_ENABLE   = 0x03U
};

struct pwm_nxp_flexio_channel_config {
	/** Flexio used pin index */
	uint8_t pin_id;
	/** Counter decrement clock prescaler */
	enum pwm_nxp_flexio_prescaler prescaler;
	/** Actual Prescaler divisor */
	uint8_t prescaler_div;
};

struct pwm_nxp_flexio_pulse_info {
	uint8_t pwm_pulse_channels;
	struct pwm_nxp_flexio_channel_config *pwm_info;
};

struct pwm_nxp_flexio_config {
	const struct device *flexio_dev;
	FLEXIO_Type *flexio_base;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pwm_nxp_flexio_pulse_info *pulse_info;
	const struct nxp_flexio_child *child;
};

struct pwm_nxp_flexio_data {
	uint32_t period_cycles[FLEXIO_MAX_PWM_CHANNELS];
	uint32_t flexio_clk;
};

static int pwm_nxp_flexio_set_cycles(const struct device *dev,
				       uint32_t channel, uint32_t period_cycles,
				       uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_nxp_flexio_config *config = dev->config;
	struct pwm_nxp_flexio_data *data = dev->data;
	flexio_timer_config_t timerConfig;
	struct pwm_nxp_flexio_channel_config *pwm_info;
	FLEXIO_Type *flexio_base = (FLEXIO_Type *)(config->flexio_base);
	struct nxp_flexio_child *child = (struct nxp_flexio_child *)(config->child);
	enum pwm_nxp_flexio_polarity polarity;

	/* Check received parameters for sanity */
	if (channel >= config->pulse_info->pwm_pulse_channels) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (period_cycles == 0) {
		LOG_ERR("Channel can not be set to inactive level");
		return -ENOTSUP;
	}

	if (FLEXIO_PWM_TIMER_CMP_MAX_VALUE <= (uint16_t)pulse_cycles) {
		LOG_ERR("Duty cycle is out of range");
		return -EINVAL;
	}

	if (FLEXIO_PWM_TIMER_CMP_MAX_VALUE <= (uint16_t)(period_cycles - pulse_cycles)) {
		LOG_ERR("low period of the cycle is out of range");
		return -EINVAL;
	}

	if (pulse_cycles > period_cycles) {
		LOG_ERR("Duty cycle cannot be greater than 100 percent");
		return -EINVAL;
	}

	pwm_info = &config->pulse_info->pwm_info[channel];

	polarity = (flags & PWM_POLARITY_INVERTED) == 0 ?
		FLEXIO_PWM_ACTIVE_HIGH : FLEXIO_PWM_ACTIVE_LOW;

	/*
	 * Adjusting the timer mode to either add modulation
	 * or pull the GPIO pin HIGH/LOW to simulate
	 * 0%  or 100% duty cycle.
	 */
	if (period_cycles == pulse_cycles) {
		polarity = FLEXIO_PWM_ACTIVE_LOW;
		timerConfig.timerMode = kFLEXIO_TimerModeDisabled;
	} else if (period_cycles && pulse_cycles == 0) {
		polarity = FLEXIO_PWM_ACTIVE_HIGH;
		timerConfig.timerMode = kFLEXIO_TimerModeDisabled;
	} else if (polarity == FLEXIO_PWM_ACTIVE_HIGH) {
		timerConfig.timerMode = kFLEXIO_TimerModeDual8BitPWM;
	} else {
		timerConfig.timerMode = kFLEXIO_TimerModeDual8BitPWMLow;
	}

	data->period_cycles[channel] = period_cycles;

	timerConfig.timerCompare = ((uint8_t)(pulse_cycles - 1U)) |
		((uint8_t)(data->period_cycles[channel] - pulse_cycles - 1U)
		 << FLEXIO_PWM_TIMCMP_CMP_UPPER_SHIFT);

	timerConfig.timerOutput = kFLEXIO_TimerOutputZeroNotAffectedByReset;
	timerConfig.timerDecrement = pwm_info->prescaler;
	timerConfig.timerStop = kFLEXIO_TimerStopBitDisabled;
	timerConfig.timerEnable = kFLEXIO_TimerEnabledAlways;
	timerConfig.timerDisable = kFLEXIO_TimerDisableNever;
	timerConfig.timerStart = kFLEXIO_TimerStartBitDisabled;
	timerConfig.timerReset = kFLEXIO_TimerResetNever;
	timerConfig.triggerSource = kFLEXIO_TimerTriggerSourceInternal;

	/* Enable the pin out for the selected timer */
	timerConfig.pinConfig = FLEXIO_PWM_TIMER_PIN_OUTPUT_ENABLE;
	timerConfig.pinPolarity = polarity;

	/* Select the pin that the selected timer will output the signal on */
	timerConfig.pinSelect = pwm_info->pin_id;

	FLEXIO_SetTimerConfig(flexio_base, child->res.timer_index[channel], &timerConfig);

#if (defined(FSL_FEATURE_FLEXIO_HAS_PIN_REGISTER) && FSL_FEATURE_FLEXIO_HAS_PIN_REGISTER)
	/* Disable pin override if active to support channels working in cases not 0% 100% */
	if (FLEXIO_GetPinOverride(flexio_base, pwm_info->pin_id)) {
		FLEXIO_ConfigPinOverride(flexio_base, pwm_info->pin_id, false);
	}
#endif
	return 0;
}

static int pwm_nxp_flexio_get_cycles_per_sec(const struct device *dev,
						uint32_t channel,
						uint64_t *cycles)
{
	const struct pwm_nxp_flexio_config *config = dev->config;
	struct pwm_nxp_flexio_data *data = dev->data;
	struct pwm_nxp_flexio_channel_config *pwm_info;

	/* If get_cycles is called directly after init */
	if (data->period_cycles[channel] == 0) {
		LOG_ERR("First set the period of this channel to a non zero value");
		return -ENOTSUP;
	}

	pwm_info = &config->pulse_info->pwm_info[channel];
	*cycles = (uint64_t)(((data->flexio_clk) * 2) /
			((data->period_cycles[channel]) * (pwm_info->prescaler_div)));

	return 0;
}

static int mcux_flexio_pwm_init(const struct device *dev)
{
	const struct pwm_nxp_flexio_config *config = dev->config;
	struct pwm_nxp_flexio_data *data = dev->data;
	flexio_timer_config_t timerConfig;
	uint8_t ch_id = 0;
	int err;
	struct pwm_nxp_flexio_channel_config *pwm_info;
	FLEXIO_Type *flexio_base = (FLEXIO_Type *)(config->flexio_base);
	struct nxp_flexio_child *child = (struct nxp_flexio_child *)(config->child);

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
					&data->flexio_clk)) {
		return -EINVAL;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	err = nxp_flexio_child_attach(config->flexio_dev, child);
	if (err < 0) {
		return err;
	}

	for (ch_id = 0; ch_id < config->pulse_info->pwm_pulse_channels; ch_id++) {
		pwm_info = &config->pulse_info->pwm_info[ch_id];

		/* Reset timer settings */
		(void)memset(&timerConfig, 0, sizeof(timerConfig));
		FLEXIO_SetTimerConfig(flexio_base, child->res.timer_index[ch_id], &timerConfig);

#if (defined(FSL_FEATURE_FLEXIO_HAS_PIN_REGISTER) && FSL_FEATURE_FLEXIO_HAS_PIN_REGISTER)
		/* Reset the value driven on the corresponding pin */
		FLEXIO_SetPinLevel(flexio_base, pwm_info->pin_id, false);
		FLEXIO_ConfigPinOverride(flexio_base, pwm_info->pin_id, false);
#endif
		/* Timer output is logic one and is not affected by timer reset */
		timerConfig.timerOutput = kFLEXIO_TimerOutputOneNotAffectedByReset;
		/* Set the timer mode to dual 8-bit counter PWM high */
		timerConfig.timerMode = kFLEXIO_TimerModeDual8BitPWM;

		/* Timer scaling factor w.r.t Flexio Clock */
		timerConfig.timerDecrement = pwm_info->prescaler;

		/* Program the PWM pulse */
		timerConfig.timerCompare = 0;

		/* Configure Timer CFG and CTL bits to support PWM mode */
		timerConfig.timerStop = kFLEXIO_TimerStopBitDisabled;
		timerConfig.timerEnable = kFLEXIO_TimerEnabledAlways;
		timerConfig.timerDisable = kFLEXIO_TimerDisableNever;
		timerConfig.timerStart = kFLEXIO_TimerStartBitDisabled;
		timerConfig.timerReset = kFLEXIO_TimerResetNever;
		timerConfig.triggerSource = kFLEXIO_TimerTriggerSourceInternal;

		/* Enable the pin out and set a default polarity for the selected timer */
		timerConfig.pinConfig = FLEXIO_PWM_TIMER_PIN_OUTPUT_ENABLE;
		timerConfig.pinPolarity = kFLEXIO_PinActiveHigh;

		/* Select the pin that the selected timer will output the signal on */
		timerConfig.pinSelect = pwm_info->pin_id;

		FLEXIO_SetTimerConfig(flexio_base, child->res.timer_index[ch_id], &timerConfig);
	}

	return 0;
}

static DEVICE_API(pwm, pwm_nxp_flexio_driver_api) = {
	.set_cycles = pwm_nxp_flexio_set_cycles,
	.get_cycles_per_sec = pwm_nxp_flexio_get_cycles_per_sec,
};

#define _FLEXIO_PWM_PULSE_GEN_CONFIG(n)								\
	{											\
		.pin_id = DT_PROP(n, pin_id),							\
		.prescaler = _CONCAT(FLEXIO_PWM_CLK_DIV_, DT_PROP(n, prescaler)),		\
		.prescaler_div = DT_PROP(n, prescaler),						\
	},

#define FLEXIO_PWM_PULSE_GEN_CONFIG(n)								\
	static struct pwm_nxp_flexio_channel_config flexio_pwm_##n##_init[] = {			\
		DT_INST_FOREACH_CHILD_STATUS_OKAY(n, _FLEXIO_PWM_PULSE_GEN_CONFIG)		\
	};											\
	static const struct pwm_nxp_flexio_pulse_info flexio_pwm_##n##_info = {			\
		.pwm_pulse_channels = ARRAY_SIZE(flexio_pwm_##n##_init),			\
		.pwm_info = flexio_pwm_##n##_init,						\
	};

#define FLEXIO_PWM_TIMER_INDEX_INIT(n)								\
	static uint8_t flexio_pwm_##n##_timer_index[ARRAY_SIZE(flexio_pwm_##n##_init)];

#define FLEXIO_PWM_CHILD_CONFIG(n)								\
	static const struct nxp_flexio_child mcux_flexio_pwm_child_##n = {			\
		.isr = NULL,									\
		.user_data = NULL,								\
		.res = {									\
			.shifter_index = NULL,							\
			.shifter_count = 0,							\
			.timer_index = (uint8_t *)flexio_pwm_##n##_timer_index,			\
			.timer_count = ARRAY_SIZE(flexio_pwm_##n##_init)			\
		}										\
	};

#define FLEXIO_PWM_PULSE_GEN_GET_CONFIG(n)							\
	.pulse_info = &flexio_pwm_##n##_info,


#define PWM_NXP_FLEXIO_PWM_INIT(n)								\
	PINCTRL_DT_INST_DEFINE(n);								\
	FLEXIO_PWM_PULSE_GEN_CONFIG(n)								\
	FLEXIO_PWM_TIMER_INDEX_INIT(n)								\
	FLEXIO_PWM_CHILD_CONFIG(n)								\
	static const struct pwm_nxp_flexio_config pwm_nxp_flexio_config_##n = {			\
		.flexio_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),					\
		.flexio_base = (FLEXIO_Type *)DT_REG_ADDR(DT_INST_PARENT(n)),			\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),					\
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(n))),			\
		.clock_subsys = (clock_control_subsys_t)DT_CLOCKS_CELL(DT_INST_PARENT(n), name),\
		.child = &mcux_flexio_pwm_child_##n,						\
		FLEXIO_PWM_PULSE_GEN_GET_CONFIG(n)						\
	};											\
												\
	static struct pwm_nxp_flexio_data pwm_nxp_flexio_data_##n;				\
	DEVICE_DT_INST_DEFINE(n,								\
			      &mcux_flexio_pwm_init,						\
			      NULL,								\
			      &pwm_nxp_flexio_data_##n,						\
			      &pwm_nxp_flexio_config_##n,					\
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,				\
			      &pwm_nxp_flexio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_NXP_FLEXIO_PWM_INIT)
