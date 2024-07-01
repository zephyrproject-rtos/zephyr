/*
 * Copyright (c) 2023 Martin Kiepfer <mrmarteng@teleschirm.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT backlight_pwm

/**
 * @file
 * @brief Backlight-PWM driver
 */

#include <zephyr/display/backlight.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/math_extras.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(backlight_pwm, CONFIG_DISPLAY_LOG_LEVEL);

struct backlight_pwm_config {
	const struct pwm_dt_spec pwm;
	const int init_level;
};

struct backlight_pwm_data {
	int level;
};

static int backlight_pwm_set_brightness(const struct device *dev, uint8_t value)
{
	const struct backlight_pwm_config *config = dev->config;
	const struct pwm_dt_spec *pwm = &config->pwm;
	struct backlight_pwm_data *data = dev->data;

	if (value > 100U) {
		return -EINVAL;
	}

	if (pwm != 0) {
		data->level = value;
		return pwm_set_pulse_dt(&config->pwm,
					(uint32_t)((uint64_t)pwm->period * value / 100U));
	}

	return -EINVAL;
}

static int backlight_pwm_on(const struct device *dev)
{
	const struct backlight_pwm_data *data = dev->data;
	const struct backlight_pwm_config *config = dev->config;

	return pwm_set_pulse_dt(&config->pwm, data->level);
}

static int backlight_pwm_off(const struct device *dev)
{
	const struct backlight_pwm_config *config = dev->config;

	return pwm_set_pulse_dt(&config->pwm, 0);
}

static int backlight_pwm_init(const struct device *dev)
{
	const struct backlight_pwm_config *config = dev->config;
	const struct pwm_dt_spec *pwm = &config->pwm;

	if (pwm != 0) {
		if (!device_is_ready(pwm->dev)) {
			LOG_ERR("%s: pwm device not ready", pwm->dev->name);
			return -ENODEV;
		}

		backlight_pwm_set_brightness(dev, config->init_level);
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int backlight_pwm_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct backlight_pwm_config *config = dev->config;

	/* switch PWM device to the new state */
	int err;
	const struct pwm_dt_spec *pwm = &config->pwm;

	LOG_DBG("PWM %p running pm action %" PRIu32, pwm->dev, action);

	err = pm_device_action_run(pwm->dev, action);
	if (err && (err != -EALREADY)) {
		LOG_DBG("Cannot switch PWM %p power state (err = %d)", pwm->dev, err);
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static const struct backlight_driver_api backlight_pwm_api = {
	.on = backlight_pwm_on,
	.off = backlight_pwm_off,
	.set_brightness = backlight_pwm_set_brightness,
};

#define BACKLIGHT_PWM_DEVICE(id)                                                                   \
                                                                                                   \
	BUILD_ASSERT(DT_INST_PROP_LEN(id, pwms) <= 1,                                              \
		     "One PWM per clock control node is supported");                               \
                                                                                                   \
	static const struct backlight_pwm_config backlight_pwm_config_##id = {                     \
		.pwm = PWM_DT_SPEC_INST_GET(id),                                                   \
		.init_level = DT_INST_PROP(id, init_level),                                        \
	};                                                                                         \
                                                                                                   \
	static struct backlight_pwm_data backlight_pwm_data_##id = {                               \
		.level = 0,                                                                        \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(id, backlight_pwm_pm_action);                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, &backlight_pwm_init, PM_DEVICE_DT_INST_GET(id),                  \
			      &backlight_pwm_data_##id, &backlight_pwm_config_##id, POST_KERNEL,   \
			      CONFIG_BACKLIGHT_PWM_INIT_PRIORITY, &backlight_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(BACKLIGHT_PWM_DEVICE)
