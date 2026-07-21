/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max2221x_pwm

#include <stdlib.h>
#include <stdint.h>
#include <zephyr/drivers/mfd/max2221x.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pwm/max2221x.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_max2221x, CONFIG_PWM_LOG_LEVEL);

struct max2221x_pwm_config {
	const struct device *parent;
};

int max2221x_get_master_chop_freq(const struct device *dev)
{
	int ret;
	uint16_t reg;
	const struct max2221x_pwm_config *config = dev->config;

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_GLOBAL_CTRL, &reg);
	if (ret) {
		LOG_ERR("Failed to read global control register");
		return ret;
	}

	switch (FIELD_GET(MAX2221X_F_PWM_M_MASK, reg)) {
	case MAX2221X_FREQ_100KHZ:
		return 100000;
	case MAX2221X_FREQ_80KHZ:
		return 80000;
	case MAX2221X_FREQ_60KHZ:
		return 60000;
	case MAX2221X_FREQ_50KHZ:
		return 50000;
	case MAX2221X_FREQ_40KHZ:
		return 40000;
	case MAX2221X_FREQ_30KHZ:
		return 30000;
	case MAX2221X_FREQ_25KHZ:
		return 25000;
	case MAX2221X_FREQ_20KHZ:
		return 20000;
	case MAX2221X_FREQ_15KHZ:
		return 15000;
	case MAX2221X_FREQ_10KHZ:
		return 10000;
	case MAX2221X_FREQ_7500HZ:
		return 7500;
	case MAX2221X_FREQ_5000HZ:
		return 5000;
	case MAX2221X_FREQ_2500HZ:
		return 2500;
	default:
		LOG_ERR("Unknown master chopping frequency");
		return -EINVAL;
	}
}

int max2221x_get_channel_freq(const struct device *dev, uint32_t channel, uint32_t *channel_freq)
{
	int ret;
	uint16_t reg, master_freq_divisor;
	uint32_t master_freq;
	const struct max2221x_pwm_config *config = dev->config;

	if (channel_freq == NULL) {
		LOG_ERR("channel_freq pointer must not be NULL");
		return -EINVAL;
	}

	ret = max2221x_get_master_chop_freq(dev);
	if (ret < 0) {
		LOG_ERR("Failed to get master chop frequency");
		return ret;
	}
	master_freq = (uint32_t)ret;

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_CFG_CTRL1(channel), &reg);
	if (ret) {
		LOG_ERR("Failed to read register for channel: %u", channel);
		return ret;
	}

	master_freq_divisor = FIELD_GET(MAX2221X_F_PWM_MASK, reg);

	switch (master_freq_divisor) {
	case MAX2221X_FREQ_M:
		*channel_freq = master_freq;
		break;
	case MAX2221X_FREQ_M_2:
		*channel_freq = master_freq / 2;
		break;
	case MAX2221X_FREQ_M_4:
		*channel_freq = master_freq / 4;
		break;
	case MAX2221X_FREQ_M_8:
		*channel_freq = master_freq / 8;
		break;
	default:
		LOG_ERR("Unknown channel frequency");
		return -EINVAL;
	}

	return 0;
}

int max2221x_calculate_duty_cycle(uint32_t pulse, uint32_t period, uint16_t *duty_cycle)
{
	if (period == 0) {
		LOG_ERR("Period must be > 0");
		return -EINVAL;
	}

	if (pulse > period) {
		LOG_ERR("Pulse width cannot be greater than period");
		return -EINVAL;
	}

	*duty_cycle = (uint16_t)(((uint64_t)pulse * UINT16_MAX) / period);

	return 0;
}

int max2221x_calculate_master_freq_divisor(uint32_t master_freq, uint32_t period, int *freq_divisor)
{
	int min_difference;
	uint32_t user_freq_hz;
	const int divisors[] = {1, 2, 4, 8};

	if (master_freq == 0) {
		LOG_ERR("Invalid input: Master frequency must be > 0");
		return -EINVAL;
	}

	if (period == 0) {
		LOG_ERR("Invalid input: Pulse width must be > 0");
		return -EINVAL;
	}

	user_freq_hz = 1000000 / period;
	*freq_divisor = divisors[0];
	min_difference = abs((int)(user_freq_hz - (master_freq / divisors[0])));

	for (int i = 1; i < ARRAY_SIZE(divisors); i++) {
		int current_freq = master_freq / divisors[i];
		int difference = abs((int)(user_freq_hz - current_freq));

		if (difference < min_difference) {
			min_difference = difference;
			*freq_divisor = divisors[i];
		}
	}

	return 0;
}

int max2221x_get_cycles_per_sec(const struct device *dev, uint32_t channel, uint64_t *cycles)
{
	int ret;
	uint32_t channel_freq;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel: %u", channel);
		return -EINVAL;
	}

	ret = max2221x_get_channel_freq(dev, channel, &channel_freq);
	if (ret < 0) {
		LOG_ERR("Failed to get channel frequency for channel: %u", channel);
		return ret;
	}

	*cycles = (uint64_t)channel_freq;

	return 0;
}

int max2221x_set_cycles(const struct device *dev, uint32_t channel, uint32_t period, uint32_t pulse,
			pwm_flags_t flags)
{
	int ret, channel_freq_divisor;
	uint16_t global_cfg, vdrnvdrduty, cfg_ctrl0, ctrl_mode;
	uint16_t duty_cycle, channel_freq_reg_value;
	uint32_t master_freq, channel_freq, valid_period;
	uint32_t min_period, max_period;
	const struct max2221x_pwm_config *config = dev->config;

	if (channel >= MAX2221X_NUM_CHANNELS) {
		LOG_ERR("Invalid channel number: %i", channel);
		return -EINVAL;
	}

	if (period == 0) {
		LOG_ERR("Period must be greater than 0");
		return -EINVAL;
	}

	if (pulse > period) {
		LOG_ERR("Pulse width cannot be greater than period");
		return -EINVAL;
	}

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_GLOBAL_CFG, &global_cfg);
	if (ret) {
		LOG_ERR("Failed to read global configuration register");
		return ret;
	}

	vdrnvdrduty = FIELD_GET(MAX2221X_VDRNVDRDUTY_MASK, global_cfg);

	ret = max2221x_reg_read(config->parent, MAX2221X_REG_CFG_CTRL0(channel), &cfg_ctrl0);
	if (ret) {
		LOG_ERR("Failed to read control mode register");
		return ret;
	}

	ctrl_mode = FIELD_GET(MAX2221X_CTRL_MODE_MASK, cfg_ctrl0);

	ret = max2221x_get_master_chop_freq(dev);
	if (ret < 0) {
		LOG_ERR("Failed to get master chop frequency");
		return ret;
	}
	master_freq = (uint32_t)ret;

	min_period = 1000000 / master_freq;
	max_period = min_period * 8;

	if (period < min_period || period > max_period) {
		LOG_ERR("Period must be between %d and %d microseconds for "
			"frequency %d Hz",
			min_period, max_period, master_freq);
		return -EINVAL;
	}

	ret = max2221x_calculate_master_freq_divisor(master_freq, period, &channel_freq_divisor);

	if (ret < 0) {
		LOG_ERR("Failed to calculate channel frequency divisor");
		return ret;
	}

	channel_freq = master_freq / (uint32_t)channel_freq_divisor;
	valid_period = (1.0 / channel_freq) * 1000000;

	switch (channel_freq_divisor) {
	case 1:
		channel_freq_reg_value = MAX2221X_FREQ_M;
		break;
	case 2:
		channel_freq_reg_value = MAX2221X_FREQ_M_2;
		break;
	case 4:
		channel_freq_reg_value = MAX2221X_FREQ_M_4;
		break;
	case 8:
		channel_freq_reg_value = MAX2221X_FREQ_M_8;
		break;
	default:
		LOG_ERR("Invalid channel frequency divisor: %d", channel_freq_divisor);
		return -EINVAL;
	}

	ret = max2221x_calculate_duty_cycle(pulse, valid_period, &duty_cycle);
	if (ret < 0) {
		LOG_ERR("Failed to calculate duty cycle");
		return ret;
	}

	if (vdrnvdrduty == 0) {
		ret = max2221x_reg_update(config->parent, MAX2221X_REG_CFG_CTRL1(channel),
					  MAX2221X_F_PWM_MASK, channel_freq_reg_value);
		if (ret) {
			LOG_ERR("Failed to write channel frequency for channel %d", channel);
			return ret;
		}

		if (ctrl_mode == 0 || ctrl_mode == 2) {
			ret = max2221x_reg_write(config->parent, MAX2221X_REG_CFG_DC_H(channel),
						 duty_cycle);
			if (ret) {
				LOG_ERR("Failed to write DC_H for channel %d", channel);
				return ret;
			}
		} else {
			LOG_ERR("Cannot set duty cycle in control mode %d for channel %d",
				ctrl_mode, channel);
		}
	}

	return 0;
}
static DEVICE_API(pwm, max2221x_pwm_api) = {
	.set_cycles = max2221x_set_cycles,
	.get_cycles_per_sec = max2221x_get_cycles_per_sec,
};

static int max2221x_pwm_init(const struct device *dev)
{
	const struct max2221x_pwm_config *config = dev->config;

	LOG_DBG("Initialize MAX2221X PWM instance %s", dev->name);

	if (!device_is_ready(config->parent)) {
		LOG_ERR("Parent device '%s' not ready", config->parent->name);
		return -ENODEV;
	}

	return 0;
}

#define PWM_MAX2221X_DEFINE(inst)                                                                  \
	static const struct max2221x_pwm_config max2221x_pwm_config_##inst = {                     \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, max2221x_pwm_init, NULL, NULL, &max2221x_pwm_config_##inst,    \
			      POST_KERNEL, CONFIG_PWM_MAX2221X_INIT_PRIORITY, &max2221x_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_MAX2221X_DEFINE);
