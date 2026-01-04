/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/actuator.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/q31.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define DT_DRV_COMPAT zephyr_servo_pwm

struct driver_data {
	int32_t pulse_ns;
};

struct driver_config {
	struct pwm_dt_spec pwm_spec;
	uint32_t pulse_min_ns;
	uint32_t pulse_max_ns;
	bool invert;
};

static int driver_api_set_setpoint(const struct device *dev, q31_t setpoint)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;

	if (dev_config->invert) {
		setpoint = SYS_Q31_INVERT(setpoint);
	}

	dev_data->pulse_ns = SYS_Q31_RANGE(setpoint,
					   dev_config->pulse_min_ns,
					   dev_config->pulse_max_ns);

	return pwm_set_pulse_dt(&dev_config->pwm_spec, dev_data->pulse_ns);
}

static DEVICE_API(actuator, driver_api) = {
	.set_setpoint = driver_api_set_setpoint,
};

static int driver_pm_resume(const struct device *dev)
{
	struct driver_data *dev_data = dev->data;
	const struct driver_config *dev_config = dev->config;
	int ret;

	ret = pm_device_runtime_get(dev_config->pwm_spec.dev);
	if (ret) {
		return ret;
	}

	return pwm_set_pulse_dt(&dev_config->pwm_spec, dev_data->pulse_ns);
}

static int driver_pm_suspend(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;
	int ret;

	ret = pwm_set_pulse_dt(&dev_config->pwm_spec, 0);
	if (ret) {
		return ret;
	}

	return pm_device_runtime_put(dev_config->pwm_spec.dev);
}

static int driver_pm_callback(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return driver_pm_resume(dev);

	case PM_DEVICE_ACTION_SUSPEND:
		return driver_pm_suspend(dev);

	default:
		return -ENOTSUP;
	}
}

static int driver_init(const struct device *dev)
{
	const struct driver_config *dev_config = dev->config;

	if (!pwm_is_ready_dt(&dev_config->pwm_spec)) {
		return -ENODEV;
	}

	return pm_device_driver_init(dev, driver_pm_callback);
}

#define DRIVER_INST_DEFINE(inst)								\
	static struct driver_data data##inst;							\
												\
	static const struct driver_config config##inst = {					\
		.pwm_spec = PWM_DT_SPEC_INST_GET(inst),						\
		.pulse_min_ns = DT_INST_PROP(inst, pulse_min_ns),				\
		.pulse_max_ns = DT_INST_PROP(inst, pulse_max_ns),				\
		.invert = DT_INST_PROP(inst, actuator_invert),					\
	};											\
												\
	PM_DEVICE_DT_INST_DEFINE(inst, driver_pm_callback);					\
												\
	DEVICE_DT_INST_DEFINE(									\
		inst,										\
		driver_init,									\
		PM_DEVICE_DT_INST_GET(inst),							\
		&data##inst,									\
		&config##inst,									\
		POST_KERNEL,									\
		CONFIG_ACTUATOR_INIT_PRIORITY,							\
		&driver_api									\
	);

DT_INST_FOREACH_STATUS_OKAY(DRIVER_INST_DEFINE)
