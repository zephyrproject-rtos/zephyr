/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pwm_leds

/**
 * @file
 * @brief PWM driven LEDs
 */

#include <zephyr/drivers/led.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/math_extras.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_pwm, CONFIG_LED_LOG_LEVEL);

struct led_pwm_config {
	int num_leds;
	const struct pwm_dt_spec *led;
	struct pm_device_runtime_reference *refs;
};

static int led_pwm_blink(const struct device *dev, uint32_t led,
			 uint32_t delay_on, uint32_t delay_off)
{
	const struct led_pwm_config *config = dev->config;
	const struct pwm_dt_spec *dt_led;
	uint32_t period_usec, pulse_usec;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	/*
	 * Convert delays (in ms) into PWM period and pulse (in us) and check
	 * for overflows.
	 */
	if (u32_add_overflow(delay_on, delay_off, &period_usec) ||
	    u32_mul_overflow(period_usec, 1000, &period_usec) ||
	    u32_mul_overflow(delay_on, 1000, &pulse_usec)) {
		return -EINVAL;
	}

	dt_led = &config->led[led];

	return pwm_set_dt(dt_led, PWM_USEC(period_usec), PWM_USEC(pulse_usec));
}

static int led_pwm_set_brightness(const struct device *dev,
				  uint32_t led, uint8_t value)
{
	const struct led_pwm_config *config = dev->config;
	const struct pwm_dt_spec *dt_led;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	dt_led = &config->led[led];

	return pwm_set_pulse_dt(&config->led[led],
			(uint32_t) ((uint64_t) dt_led->period * value / LED_BRIGHTNESS_MAX));
}

static int led_pwm_request_leds(const struct device *dev, bool request)
{
	const struct led_pwm_config *config = dev->config;
	const struct pwm_dt_spec *led;
	struct pm_device_runtime_reference *ref;
	const char *request_str = request ? "request" : "release";
	int ret = 0;
	int err;

	for (size_t i = 0; i < config->num_leds; i++) {
		led = &config->led[i];
		ref = &config->refs[i];

		LOG_DBG("PWM %p %s", led->dev, request_str);

		if (request) {
			err = pm_device_runtime_request(led->dev, ref);
		} else {
			err = pm_device_runtime_release(led->dev, ref);
		}

		if (err) {
			LOG_DBG("PWM %p %s failed (err = %d)", led->dev, request_str, err);
			ret = -EIO;
		}
	}

	return ret;
}

static int led_pwm_resume(const struct device *dev)
{
	int ret;

	ret = led_pwm_request_leds(dev, true);
	if (ret) {
		(void)led_pwm_request_leds(dev, false);
	}

	return ret;
}

static int led_pwm_suspend(const struct device *dev)
{
	int ret;

	ret = led_pwm_request_leds(dev, false);
	if (ret) {
		(void)led_pwm_request_leds(dev, true);
	}

	return ret;
}

static int led_pwm_pm_action(const struct device *dev,
			     enum pm_device_action action)
{
	if (action == PM_DEVICE_ACTION_RESUME) {
		return led_pwm_resume(dev);
	} else if (action == PM_DEVICE_ACTION_SUSPEND) {
		return led_pwm_suspend(dev);
	}

	return -ENOTSUP;
}

static int led_pwm_init(const struct device *dev)
{
	const struct led_pwm_config *config = dev->config;
	int i;

	if (!config->num_leds) {
		LOG_ERR("%s: no LEDs found (DT child nodes missing)",
			dev->name);
		return -ENODEV;
	}

	for (i = 0; i < config->num_leds; i++) {
		const struct pwm_dt_spec *led = &config->led[i];

		if (!device_is_ready(led->dev)) {
			LOG_ERR("%s: pwm device not ready", led->dev->name);
			return -ENODEV;
		}
	}

	return pm_device_driver_init(dev, led_pwm_pm_action);
}

static DEVICE_API(led, led_pwm_api) = {
	.blink		= led_pwm_blink,
	.set_brightness	= led_pwm_set_brightness,
};

#define LED_PWM_REF_INIT(id) \
	PM_DEVICE_RUNTIME_REFERENCE_INIT()

#define LED_PWM_DEVICE(id)					\
								\
static const struct pwm_dt_spec led_pwm_##id[] = {		\
	DT_INST_FOREACH_CHILD_SEP(id, PWM_DT_SPEC_GET, (,))	\
};								\
								\
static struct pm_device_runtime_reference led_pwm_refs##id[] = {\
	DT_INST_FOREACH_CHILD_SEP(id, LED_PWM_REF_INIT, (,))	\
};								\
								\
static const struct led_pwm_config led_pwm_config_##id = {	\
	.num_leds	= ARRAY_SIZE(led_pwm_##id),		\
	.led		= led_pwm_##id,				\
	.refs		= led_pwm_refs##id,			\
};								\
								\
PM_DEVICE_DT_INST_DEFINE(id, led_pwm_pm_action);		\
								\
DEVICE_DT_INST_DEFINE(id, &led_pwm_init,			\
		      PM_DEVICE_DT_INST_GET(id), NULL,		\
		      &led_pwm_config_##id, POST_KERNEL,	\
		      CONFIG_LED_INIT_PRIORITY, &led_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(LED_PWM_DEVICE)
