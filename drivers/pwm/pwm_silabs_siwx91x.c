/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/types.h>
#include "sl_si91x_pwm.h"

#define DT_DRV_COMPAT silabs_siwx91x_pwm

#define PWM_CHANNELS  4
#define DEFAULT_VALUE 0xFF

struct pwm_siwx91x_channel_config {
	uint8_t duty_cycle;
	uint32_t frequency;
	bool is_chan_active;
};

struct pwm_siwx91x_config {
	/* Pointer to the clock device structure */
	const struct device *clock_dev;
	/* Clock control subsystem */
	clock_control_subsys_t clock_subsys;
	/* Pointer to the pin control device configuration */
	const struct pinctrl_dev_config *pcfg;
	/* Prescaler information of the channels */
	uint8_t ch_prescaler[PWM_CHANNELS];
	/* Common PWM polarity for all the channels */
	uint8_t pwm_polarity;
};

struct pwm_siwx91x_data {
	struct pwm_siwx91x_channel_config pwm_channel_cfg[PWM_CHANNELS];
};

/* Function to convert prescaler value to a programmable reg value */
static int siwx91x_prescale_convert(uint8_t prescale)
{
	switch (prescale) {
	case 1:
		return SL_TIME_PERIOD_PRESCALE_1;
	case 2:
		return SL_TIME_PERIOD_PRESCALE_2;
	case 4:
		return SL_TIME_PERIOD_PRESCALE_4;
	case 8:
		return SL_TIME_PERIOD_PRESCALE_8;
	case 16:
		return SL_TIME_PERIOD_PRESCALE_16;
	case 32:
		return SL_TIME_PERIOD_PRESCALE_32;
	case 64:
		return SL_TIME_PERIOD_PRESCALE_64;
	default:
		return -EINVAL;
	}
}

/* Program PWM channel with the default configurations */
static int siwx91x_default_channel_config(const struct device *dev, uint32_t channel)
{
	const struct pwm_siwx91x_config *config = dev->config;
	int prescale_reg_value = siwx91x_prescale_convert(config->ch_prescaler[channel]);
	int ret;

	if (prescale_reg_value < 0) {
		return -EINVAL;
	}

	ret = sl_si91x_pwm_set_output_mode(SL_MODE_INDEPENDENT, channel);
	if (ret) {
		return -EINVAL;
	}

	ret = sl_si91x_pwm_set_base_timer_mode(SL_FREE_RUN_MODE, channel);
	if (ret) {
		return -EINVAL;
	}

	ret = sl_si91x_pwm_control_base_timer(SL_BASE_TIMER_EACH_CHANNEL);
	if (ret) {
		return -EINVAL;
	}

	ret = sl_si91x_pwm_control_period(SL_TIME_PERIOD_POSTSCALE_1_1, prescale_reg_value,
					  channel);
	if (ret) {
		return -EINVAL;
	}

	return 0;
}

static int pwm_siwx91x_set_cycles(const struct device *dev, uint32_t channel,
				  uint32_t period_cycles, uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_siwx91x_config *config = dev->config;
	struct pwm_siwx91x_data *data = dev->data;
	uint32_t prev_period;
	uint32_t duty_cycle;
	int ret;

	if (channel >= ARRAY_SIZE(data->pwm_channel_cfg)) {
		return -EINVAL;
	}

	if (config->pwm_polarity != flags) {
		/* Polarity mismatch */
		return -ENOTSUP;
	}

	if (data->pwm_channel_cfg[channel].is_chan_active == false) {
		/* Configure the channel with default parameters */
		ret = siwx91x_default_channel_config(dev, channel);
		if (ret) {
			return -EINVAL;
		}
	}

	ret = sl_si91x_pwm_get_time_period(channel, (uint16_t *)&prev_period);
	if (ret) {
		return -EINVAL;
	}

	if (period_cycles != prev_period) {
		ret = sl_si91x_pwm_set_time_period(channel, period_cycles, 0);
		if (ret) {
			/* Programmed value must be out of range (>65535) */
			return -EINVAL;
		}
	}

	/* Calculate the duty cycle */
	duty_cycle = pulse_cycles * 100;
	duty_cycle /= period_cycles;

	if (duty_cycle != data->pwm_channel_cfg[channel].duty_cycle) {
		ret = sl_si91x_pwm_set_duty_cycle(pulse_cycles, channel);
		if (ret) {
			return -EINVAL;
		}
		data->pwm_channel_cfg[channel].duty_cycle = duty_cycle;
	}

	if (data->pwm_channel_cfg[channel].is_chan_active == false) {
		/* Start PWM after configuring the channel for first time */
		ret = sl_si91x_pwm_start(channel);
		if (ret) {
			return -EINVAL;
		}
		data->pwm_channel_cfg[channel].is_chan_active = true;
	}

	return 0;
}

static int pwm_siwx91x_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					  uint64_t *cycles)
{
	struct pwm_siwx91x_data *data = dev->data;

	if (channel >= ARRAY_SIZE(data->pwm_channel_cfg)) {
		return -EINVAL;
	}

	*cycles = (uint64_t)data->pwm_channel_cfg[channel].frequency;

	return 0;
}

static int pwm_siwx91x_init(const struct device *dev)
{
	const struct pwm_siwx91x_config *config = dev->config;
	struct pwm_siwx91x_data *data = dev->data;
	bool polarity_inverted = (config->pwm_polarity == PWM_POLARITY_INVERTED);
	uint32_t pwm_frequency;
	int ret;

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret) {
		return ret;
	}

	ret = clock_control_get_rate(config->clock_dev, config->clock_subsys, &pwm_frequency);
	if (ret) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	ARRAY_FOR_EACH(data->pwm_channel_cfg, i) {
		data->pwm_channel_cfg[i].frequency = pwm_frequency / config->ch_prescaler[i];
	}

	ret = sl_si91x_pwm_set_output_polarity(polarity_inverted, !polarity_inverted);
	if (ret) {
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(pwm, pwm_siwx91x_driver_api) = {
	.set_cycles = pwm_siwx91x_set_cycles,
	.get_cycles_per_sec = pwm_siwx91x_get_cycles_per_sec,
};

#define SIWX91X_PWM_INIT(inst)                                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static struct pwm_siwx91x_data pwm_siwx91x_data_##inst;                                    \
	static const struct pwm_siwx91x_config pwm_config_##inst = {                               \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_PHA(inst, clocks, clkid),          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.ch_prescaler = DT_INST_PROP(inst, silabs_ch_prescaler),                           \
		.pwm_polarity = DT_INST_PROP(inst, silabs_pwm_polarity),                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &pwm_siwx91x_init, NULL, &pwm_siwx91x_data_##inst,             \
			      &pwm_config_##inst, PRE_KERNEL_1, CONFIG_PWM_INIT_PRIORITY,          \
			      &pwm_siwx91x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_PWM_INIT)
