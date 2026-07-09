/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT fan_pwm

#include <zephyr/drivers/fan.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(fan_pwm, CONFIG_FAN_LOG_LEVEL);

struct fan_pwm_config {
	struct pwm_dt_spec pwm;
};

struct fan_pwm_data {
	struct k_mutex lock;
	uint8_t speed_percent;
};

static int fan_pwm_set_speed(const struct device *dev, uint8_t percent)
{
	const struct fan_pwm_config *cfg = dev->config;
	struct fan_pwm_data *data = dev->data;
	uint32_t pulse_ns;
	int ret;

	if (percent > FAN_SPEED_MAX) {
		return -EINVAL;
	}

	pulse_ns = (uint32_t)((uint64_t)cfg->pwm.period * percent / FAN_SPEED_MAX);

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = pwm_set_dt(&cfg->pwm, cfg->pwm.period, pulse_ns);
	if (ret == 0) {
		data->speed_percent = percent;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static int fan_pwm_get_speed(const struct device *dev, uint8_t *percent)
{
	struct fan_pwm_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	*percent = data->speed_percent;
	k_mutex_unlock(&data->lock);

	return 0;
}

static int fan_pwm_init(const struct device *dev)
{
	const struct fan_pwm_config *cfg = dev->config;
	struct fan_pwm_data *data = dev->data;

	if (!pwm_is_ready_dt(&cfg->pwm)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->pwm.dev);
		return -ENODEV;
	}

	k_mutex_init(&data->lock);
	data->speed_percent = CONFIG_FAN_PWM_INITIAL_DUTY_CYCLE;

	return fan_pwm_set_speed(dev, CONFIG_FAN_PWM_INITIAL_DUTY_CYCLE);
}

static DEVICE_API(fan, fan_pwm_api) = {
	.set_speed = fan_pwm_set_speed,
	.get_speed = fan_pwm_get_speed,
};

#define FAN_PWM_INIT(inst)								\
	static const struct fan_pwm_config fan_pwm_cfg_##inst = {			\
		.pwm = PWM_DT_SPEC_INST_GET(inst),					\
	};										\
	static struct fan_pwm_data fan_pwm_data_##inst;					\
	DEVICE_DT_INST_DEFINE(inst, fan_pwm_init, NULL,					\
			      &fan_pwm_data_##inst, &fan_pwm_cfg_##inst,		\
			      POST_KERNEL, CONFIG_FAN_INIT_PRIORITY,			\
			      &fan_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(FAN_PWM_INIT)
