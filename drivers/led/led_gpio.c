/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_leds

/**
 * @file
 * @brief GPIO driven LEDs
 */

#include <zephyr/drivers/led.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/dt-bindings/led/led.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_gpio, CONFIG_LED_LOG_LEVEL);

struct led_gpio_child {
	const struct gpio_dt_spec *gpio;
	struct led_info info;
};

struct led_gpio_config {
	size_t num_leds;
	const struct led_gpio_child *led;
};

static int led_gpio_get_info(const struct device *dev, uint32_t led, const struct led_info **info)
{
	const struct led_gpio_config *config = dev->config;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	*info = &config->led[led].info;

	return 0;
}

static int led_gpio_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{

	const struct led_gpio_config *config = dev->config;
	const struct led_gpio_child *led_child;
	int err = 0;

	if ((led >= config->num_leds) || (value > 100)) {
		return -EINVAL;
	}

	for (uint8_t i = 0; i < config->led[led].info.num_colors && !err; i++) {
		led_child = &config->led[led];

		err = gpio_pin_set_dt(&led_child->gpio[i], value > 0);

		if (err) {
			LOG_ERR("Cannot set GPIO (err %d)", err);
		}
	}

	return err;
}

static int led_gpio_on(const struct device *dev, uint32_t led)
{
	return led_gpio_set_brightness(dev, led, 100);
}

static int led_gpio_off(const struct device *dev, uint32_t led)
{
	return led_gpio_set_brightness(dev, led, 0);
}

static int led_gpio_set_color(const struct device *dev, uint32_t led, uint8_t num_colors,
			      const uint8_t *color)
{
	const struct led_gpio_config *config = dev->config;
	const struct led_gpio_child *led_child;
	int err = 0;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	led_child = &config->led[led];

	if (num_colors > led_child->info.num_colors) {
		return -EINVAL;
	}

	for (uint8_t i = 0; i < num_colors && !err; i++) {
		err = gpio_pin_set_dt(&led_child->gpio[i], color[i]);

		if (err) {
			LOG_ERR("Cannot set GPIO (err %d)", err);
		}
	}

	return err;
}

static int led_gpio_init(const struct device *dev)
{
	const struct led_gpio_config *config = dev->config;
	const struct gpio_dt_spec *led;
	int err = 0;

	if (!config->num_leds) {
		LOG_ERR("%s: no LEDs found (DT child nodes missing)", dev->name);
		err = -ENODEV;
	}

	for (size_t i = 0; (i < config->num_leds) && !err; i++) {
		for (uint8_t j = 0; (j < config->led[i].info.num_colors) && !err; j++) {
			led = &config->led[i].gpio[j];

			if (device_is_ready(led->port)) {
				err = gpio_pin_configure_dt(led, GPIO_OUTPUT_INACTIVE);

				if (err) {
					LOG_ERR("Cannot configure GPIO (err %d)", err);
				}
			} else {
				LOG_ERR("%s: GPIO device not ready", dev->name);
				err = -ENODEV;
			}
		}
	}

	return err;
}

static const struct led_driver_api led_gpio_api = {
	.get_info	= led_gpio_get_info,
	.on		= led_gpio_on,
	.off		= led_gpio_off,
	.set_brightness	= led_gpio_set_brightness,
	.set_color	= led_gpio_set_color,
};

#define LED_GPIO_CHILD_GPIO_GET(idx, node_id)			\
	GPIO_DT_SPEC_GET_BY_IDX(node_id, gpios, idx)

#define LED_GPIO_CHILD_GPIO(node_id, i)				\
static const struct gpio_dt_spec gpio_dt_spec_##i##_##node_id[] = { \
	LISTIFY(DT_PROP_LEN(node_id, gpios),			\
		LED_GPIO_CHILD_GPIO_GET, (,), node_id)		\
};

#define LED_GPIO_CHILD_COLOR_MAPPING(node_id, i)		\
static const uint8_t color_mapping_##i##_##node_id[] =		\
	DT_PROP_OR(node_id, color_mapping, {LED_COLOR_ID_WHITE});

#define LED_GPIO_CHILD(node_id, i)				\
	{							\
		.gpio = gpio_dt_spec_##i##_##node_id,		\
		.info = {					\
			.label = DT_PROP_OR(node_id, label, NULL), \
			.index = DT_NODE_CHILD_IDX(node_id),	\
			.num_colors = DT_PROP_LEN_OR(node_id, color_mapping, 1), \
			.color_mapping = color_mapping_##i##_##node_id, \
		},						\
	}

#define LED_GPIO_DEVICE(i)					\
								\
DT_INST_FOREACH_CHILD_VARGS(i, LED_GPIO_CHILD_GPIO, i)		\
DT_INST_FOREACH_CHILD_VARGS(i, LED_GPIO_CHILD_COLOR_MAPPING, i)	\
								\
static const struct led_gpio_child led_gpio_child_##i[] = {	\
	DT_INST_FOREACH_CHILD_SEP_VARGS(i, LED_GPIO_CHILD, (,), i) \
};								\
								\
static const struct led_gpio_config led_gpio_config_##i = {	\
	.num_leds	= ARRAY_SIZE(led_gpio_child_##i),	\
	.led		= led_gpio_child_##i,			\
};								\
								\
DEVICE_DT_INST_DEFINE(i, &led_gpio_init, NULL,			\
		      NULL, &led_gpio_config_##i,		\
		      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,	\
		      &led_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(LED_GPIO_DEVICE)
