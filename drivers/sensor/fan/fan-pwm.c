/*
 * Copyright (c) 2026 Kasper Sloth
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_pwm_fan

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/fan.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fan_pwm, CONFIG_FAN_LOG_LEVEL);

struct pwm_fan_config {
	struct pwm_dt_spec pwm;
	const struct device *tach; /* May be NULL */
};

struct pwm_fan_data {
	uint8_t percent;
	uint32_t rpm;
};

static int pwm_fan_set_percent(const struct device *dev, uint8_t percent)
{
	const struct pwm_fan_config *config = dev->config;
	struct pwm_fan_data *data = dev->data;
	uint32_t pulse;
	int ret;

	if (percent > 100U) {
		return -EINVAL;
	}

	pulse = (uint32_t)((uint64_t)config->pwm.period * percent / 100U);

	ret = pwm_set_pulse_dt(&config->pwm, pulse);
	if (ret != 0) {
		return ret;
	}

	data->percent = percent;

	return 0;
}

static int pwm_fan_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_RPM && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	if ((enum sensor_attribute_fan)attr != SENSOR_ATTR_FAN_SPEED) {
		return -ENOTSUP;
	}

	if (val->val1 < 0 || val->val1 > 100) {
		return -EINVAL;
	}

	return pwm_fan_set_percent(dev, (uint8_t)val->val1);
}

static int pwm_fan_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, struct sensor_value *val)
{
	const struct pwm_fan_data *data = dev->data;

	if (chan != SENSOR_CHAN_RPM && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	if ((enum sensor_attribute_fan)attr != SENSOR_ATTR_FAN_SPEED) {
		return -ENOTSUP;
	}

	val->val1 = data->percent;
	val->val2 = 0;

	return 0;
}

static int pwm_fan_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct pwm_fan_config *config = dev->config;
	struct pwm_fan_data *data = dev->data;
	struct sensor_value val;
	int ret;

	if (chan != SENSOR_CHAN_RPM && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	if (config->tach == NULL) {
		return -ENOTSUP;
	}

	ret = sensor_sample_fetch_chan(config->tach, SENSOR_CHAN_RPM);
	if (ret != 0) {
		return ret;
	}

	ret = sensor_channel_get(config->tach, SENSOR_CHAN_RPM, &val);
	if (ret != 0) {
		return ret;
	}

	data->rpm = (uint32_t)val.val1;

	return 0;
}

static int pwm_fan_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct pwm_fan_data *data = dev->data;

	if (chan != SENSOR_CHAN_RPM) {
		return -ENOTSUP;
	}

	val->val1 = data->rpm;
	val->val2 = 0;

	return 0;
}

static DEVICE_API(sensor, fan_driver_api) = {
	.attr_set = pwm_fan_attr_set,
	.attr_get = pwm_fan_attr_get,
	.sample_fetch = pwm_fan_sample_fetch,
	.channel_get = pwm_fan_channel_get,
};

static int pwm_fan_init(const struct device *dev)
{
	const struct pwm_fan_config *config = dev->config;

	if (!pwm_is_ready_dt(&config->pwm)) {
		LOG_ERR("PWM device %s is not ready", config->pwm.dev->name);
		return -ENODEV;
	}

	if (config->tach != NULL && !device_is_ready(config->tach)) {
		LOG_ERR("Tachometer device %s is not ready", config->tach->name);
		return -ENODEV;
	}

	return pwm_fan_set_percent(dev, CONFIG_FAN_PWM_START_PERCENT);
}

#define PWM_FAN_INIT(inst)                                                                         \
	static const struct pwm_fan_config pwm_fan_cfg_##inst = {                                  \
		.pwm = PWM_DT_SPEC_INST_GET(inst),                                                 \
		.tach = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, tach)),                        \
	};                                                                                         \
	static struct pwm_fan_data pwm_fan_data_##inst = {                                         \
		.percent = CONFIG_FAN_PWM_START_PERCENT,                                           \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, pwm_fan_init, NULL, &pwm_fan_data_##inst,               \
				     &pwm_fan_cfg_##inst, POST_KERNEL, CONFIG_FAN_INIT_PRIORITY,   \
				     &fan_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_FAN_INIT)
