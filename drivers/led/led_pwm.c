/*
 * Copyright (c) 2020 Seagate Technology LLC
 * Copyright (c) 2024 Nick Ward <nix.ward@gmail.com>
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
#include <zephyr/kernel.h>
#include <zephyr/sys/math_extras.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_pwm, CONFIG_LED_LOG_LEVEL);

struct led_pwm_child {
	uint32_t num_pwms;
	const struct pwm_dt_spec *pwm;
	struct led_info info;
};

struct led_pwm_config {
	uint32_t num_leds;
	const struct led_pwm_child *child;
};

static int led_pwm_blink(const struct device *dev, uint32_t led,
			 uint32_t delay_on, uint32_t delay_off)
{
	const struct led_pwm_config *config = dev->config;
	uint32_t period_usec, pulse_usec;
	const struct pwm_dt_spec *pwm;
	int pwm_ret = 0;
	int ret;

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

	for (int i = 0; i < config->child[led].num_pwms; i++) {
		pwm = &config->child[led].pwm[i];
		ret = pwm_set_dt(pwm, PWM_USEC(period_usec), PWM_USEC(pulse_usec));
		if (ret != 0) {
			LOG_ERR("LED child '%s' pwm[%u] set pulse: %d",
				config->child[led].info.label, i, ret);
			pwm_ret = ret; /* return the last error to occur */
		}
	}

	return pwm_ret;
}

static int led_pwm_get_info(const struct device *dev, uint32_t led, const struct led_info **info)
{
	const struct led_pwm_config *config = dev->config;

	if (led >= config->num_leds) {
		*info = NULL;
		return -EINVAL;
	}

	*info = &config->child[led].info;

	return 0;
}

static int led_pwm_set_color(const struct device *dev, uint32_t led, uint8_t num_colors,
			     const uint8_t *color)
{
	const struct led_pwm_config *config = dev->config;
	const struct led_pwm_child *child;
	const struct pwm_dt_spec *pwm;
	int pwm_ret = 0;
	uint32_t pulse;
	int ret;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	child = &config->child[led];

	if (num_colors != child->info.num_colors) {
		LOG_ERR("Invalid number of colors: %u, supports: %u", num_colors,
			child->info.num_colors);
		return -EINVAL;
	}

	for (int i = 0; i < config->child[led].num_pwms; i++) {
		pwm = &config->child[led].pwm[i];
		pulse = (uint64_t)pwm->period * color[i] / 100;
		ret = pwm_set_pulse_dt(pwm, pulse);
		if (ret != 0) {
			LOG_ERR("LED child '%s' pwm[%u] set pulse: %d",
				config->child[led].info.label, i, ret);
			pwm_ret = ret; /* return the last error to occur */
		}
	}

	return pwm_ret;
}


static int led_pwm_set_brightness(const struct device *dev,
				  uint32_t led, uint8_t value)
{
	const struct led_pwm_config *config = dev->config;
	const struct pwm_dt_spec *pwm;
	int pwm_ret = 0;
	uint32_t pulse;
	int ret;

	if (led >= config->num_leds || value > 100) {
		return -EINVAL;
	}

	for (int i = 0; i < config->child[led].num_pwms; i++) {
		pwm = &config->child[led].pwm[i];
		pulse = (uint64_t)pwm->period * value / 100;
		ret = pwm_set_pulse_dt(pwm, pulse);
		if (ret != 0) {
			LOG_ERR("LED child '%s' pwm[%u] set pulse: %d",
				config->child[led].info.label, i, ret);
			pwm_ret = ret; /* return the last error to occur */
		}
	}

	return pwm_ret;
}

static int led_pwm_on(const struct device *dev, uint32_t led)
{
	return led_pwm_set_brightness(dev, led, 100);
}

static int led_pwm_off(const struct device *dev, uint32_t led)
{
	return led_pwm_set_brightness(dev, led, 0);
}

static int led_pwm_init(const struct device *dev)
{
	const struct led_pwm_config *config = dev->config;
	const struct led_pwm_child *child;
	const struct pwm_dt_spec *pwm;

	for (int led = 0; led < config->num_leds; led++) {
		child = &config->child[led];
		for (int i = 0; i < child->num_pwms; i++) {
			pwm = &child->pwm[i];
			if (!pwm_is_ready_dt(pwm)) {
				LOG_ERR("LED child '%s' pwm[%u]: device not ready",
					child->info.label, i);
				return -ENODEV;
			}
		}
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int led_pwm_pm_action(const struct device *dev,
			     enum pm_device_action action)
{
	const struct led_pwm_config *config = dev->config;
	const struct led_pwm_child *child;
	const struct pwm_dt_spec *pwm;
	int ret;

	/* Switch all underlying PWM devices to the new state */
	for (int led = 0; led < config->num_leds; led++) {
		child = &config->child[led];
		for (int i = 0; i < child->num_pwms; i++) {
			pwm = &child->pwm[i];
			LOG_DBG("PWM %p running pm action %" PRIu32, pwm->dev, action);

			ret = pm_device_action_run(pwm->dev, action);
			if ((ret != 0) && (ret != -EALREADY)) {
				LOG_ERR("Cannot switch PWM %p power state", pwm->dev);
			}
		}
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static const struct led_driver_api led_pwm_api = {
	.on		= led_pwm_on,
	.off		= led_pwm_off,
	.blink		= led_pwm_blink,
	.set_brightness	= led_pwm_set_brightness,
	.get_info	= led_pwm_get_info,
	.set_color	= led_pwm_set_color,
};

#define CHILD_LED_PWMS(led_node_id)                                                                \
	static const struct pwm_dt_spec pwm_##led_node_id[] = {                                    \
		DT_FOREACH_PROP_ELEM_SEP(led_node_id, pwms, PWM_DT_SPEC_GET_BY_PROP_AND_IDX, (,))};

#define CHILD_LED_COLOR_MAPPING(led_node_id)                                                       \
	const uint8_t color_mapping_##led_node_id[] = DT_PROP(led_node_id, color_mapping);

#define CHILD_LED_INFO(led_node_id)                                                                \
	{                                                                                          \
		.num_pwms = ARRAY_SIZE(pwm_##led_node_id),                                         \
		.pwm = pwm_##led_node_id,                                                          \
		.info =                                                                            \
			{                                                                          \
				.label = DT_PROP(led_node_id, label),                              \
				.index = DT_PROP(led_node_id, index),                              \
				.num_colors = DT_PROP_LEN(led_node_id, color_mapping),             \
				.color_mapping = color_mapping_##led_node_id,                      \
			},                                                                         \
	},

#define LED_PWM_DEVICE(inst)                                                                       \
                                                                                                   \
	DT_INST_FOREACH_CHILD(inst, CHILD_LED_COLOR_MAPPING)                                       \
                                                                                                   \
	DT_INST_FOREACH_CHILD(inst, CHILD_LED_PWMS);                                               \
                                                                                                   \
	static const struct led_pwm_child led_child_##inst[] = {                                   \
		DT_INST_FOREACH_CHILD(inst, CHILD_LED_INFO)};                                      \
                                                                                                   \
	BUILD_ASSERT(ARRAY_SIZE(led_child_##inst) > 0,                                             \
		     "An led_pwm device must have child nodes\n");                                 \
                                                                                                   \
	static const struct led_pwm_config led_pwm_config_##inst = {                               \
		.num_leds = ARRAY_SIZE(led_child_##inst),                                          \
		.child = led_child_##inst,                                                         \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, led_pwm_pm_action);                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &led_pwm_init, PM_DEVICE_DT_INST_GET(inst), NULL,              \
			      &led_pwm_config_##inst, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,       \
			      &led_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(LED_PWM_DEVICE)
