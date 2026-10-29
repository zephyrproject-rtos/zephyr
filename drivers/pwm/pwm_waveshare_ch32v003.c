/*
 * Copyright (c) 2026 Kim Bøndergaard <kim@fam-boendergaard.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT waveshare_ch32v003_pwm

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mfd/waveshare_ch32v003.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(pwm_ch32v003, CONFIG_PWM_LOG_LEVEL);

struct ch32v003_pwm_config {
	const struct device *parent;
};

struct ch32v003_pwm_data {
	struct k_mutex *lock;
	uint32_t period_cycles;
};

int ch32v003_set_pwm_duty(const struct device *dev, uint8_t duty)
{
	const struct ch32v003_pwm_config *cfg = dev->config;
	struct ch32v003_pwm_data *data = dev->data;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(data->lock, K_FOREVER);

	ret = ch32v003_write_reg8(cfg->parent, CH32V003_REG_PWM, duty);
	k_mutex_unlock(data->lock);

	if (ret < 0) {
		return -EIO;
	}

	return 0;
}

static int ch32v003_pwm_set_cycles(const struct device *dev, uint32_t channel,
				   uint32_t period_cycles, uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct ch32v003_pwm_config *cfg = dev->config;
	struct ch32v003_pwm_data *data = dev->data;
	uint8_t duty;

	if (channel != 0U) {
		return -ENOTSUP;
	}

	if (period_cycles == 0U) {
		return -EINVAL;
	}

	if (pulse_cycles > period_cycles) {
		return -EINVAL;
	}

	duty = (uint8_t)(((uint64_t)pulse_cycles * 255U) / period_cycles);
	if (data->period_cycles != period_cycles) {
		if (data->period_cycles != 0U) {
			LOG_WRN("Changing period cycles from %u to %u", data->period_cycles,
				period_cycles);
		}
		data->period_cycles = period_cycles;
	}

	if (flags & PWM_POLARITY_INVERTED) {
		duty = 255U - duty;
	}

	return ch32v003_set_pwm_duty(cfg->parent, duty);
}

static int ch32v003_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					   uint64_t *cycles)
{
	ARG_UNUSED(dev);

	if (channel != 0U) {
		return -ENOTSUP;
	}

	*cycles = 1000000000ULL;

	return 0;
}

static int ch32v003_pwm_init(const struct device *dev)
{
	const struct ch32v003_pwm_config *cfg = dev->config;
	struct ch32v003_pwm_data *data = dev->data;

	if (!device_is_ready(cfg->parent)) {
		LOG_ERR("Parent CH32V003 device is not ready");
		return -ENODEV;
	}

	data->lock = ch32v003_get_lock(cfg->parent);

	return 0;
}

static DEVICE_API(pwm, ch32v003_pwm_api) = {
	.set_cycles = ch32v003_pwm_set_cycles,
	.get_cycles_per_sec = ch32v003_pwm_get_cycles_per_sec,
};

#define CH32V003_PWM_INST_DEFINE(n)                                                                \
	static const struct ch32v003_pwm_config ch32v003_pwm_cfg_##n = {                           \
		.parent = DEVICE_DT_GET(DT_INST_BUS(n)),                                             \
	};                                                                                         \
	static struct ch32v003_pwm_data ch32v003_pwm_data_##n = {                                  \
		.period_cycles = 0U,                                                               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, ch32v003_pwm_init, NULL, &ch32v003_pwm_data_##n,                  \
			      &ch32v003_pwm_cfg_##n, POST_KERNEL,                                  \
			      CONFIG_PWM_WAVESHARE_CH32V003_INIT_PRIORITY, &ch32v003_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(CH32V003_PWM_INST_DEFINE)
