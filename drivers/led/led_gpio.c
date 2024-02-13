/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2024 Simon Guinot <simon.guinot@sequanux.org>
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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_gpio, CONFIG_LED_LOG_LEVEL);

struct led_gpio {
	size_t num_gpios;
	const struct gpio_dt_spec *gpio;
};

struct led_gpio_config {
	size_t num_leds;
	const struct led_gpio *led;
};

static int led_gpio_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{

	const struct led_gpio_config *config = dev->config;
	const struct led_gpio *led_data;

	if ((led >= config->num_leds) || (value > 100)) {
		return -EINVAL;
	}

	led_data = &config->led[led];

	if (led_data->num_gpios != 1) {
		return -ENOTSUP;
	}

	return gpio_pin_set_dt(&led_data->gpio[0], value > 0);
}

static int led_gpio_on(const struct device *dev, uint32_t led)
{
	return led_gpio_set_brightness(dev, led, 100);
}

static int led_gpio_off(const struct device *dev, uint32_t led)
{
	return led_gpio_set_brightness(dev, led, 0);
}

static int led_gpio_set_color(const struct device *dev, uint32_t led,
			      uint8_t num_colors, const uint8_t *color)
{
	const struct led_gpio_config *config = dev->config;
	const struct led_gpio *led_data;
	int err = 0;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	led_data = &config->led[led];

	if (led_data->num_gpios != num_colors) {
		return -EINVAL;
	}

	for (size_t i = 0; (i < num_colors) && !err; i++) {
		err = gpio_pin_set_dt(&led_data->gpio[i], color[i] > 0);
	}

	return err;
}

static int led_gpio_init(const struct device *dev)
{
	const struct led_gpio_config *config = dev->config;
	int err = 0;

	if (!config->num_leds) {
		LOG_ERR("%s: no LEDs found (DT child nodes missing)", dev->name);
		err = -ENODEV;
	}

	for (size_t i = 0; (i < config->num_leds) && !err; i++) {
		const struct led_gpio *led = &config->led[i];

		for (size_t j = 0; (j < led->num_gpios) && !err; j++) {
			const struct gpio_dt_spec *gpio = &led->gpio[j];

			if (gpio_is_ready_dt(gpio)) {
				err = gpio_pin_configure_dt(gpio, GPIO_OUTPUT_INACTIVE);
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
	.on		= led_gpio_on,
	.off		= led_gpio_off,
	.set_brightness	= led_gpio_set_brightness,
	.set_color	= led_gpio_set_color,
};

#define LED_GPIO_DT_SPEC(led_node_id)					\
	static const struct gpio_dt_spec gpio_##led_node_id[] =	{	\
		DT_FOREACH_PROP_ELEM_SEP(led_node_id, gpios,		\
					 GPIO_DT_SPEC_GET_BY_IDX, (,))	\
	};

#define LED_GPIO(led_node_id)						\
	{								\
		.num_gpios	= DT_PROP_LEN(led_node_id, gpios),	\
		.gpio		= gpio_##led_node_id,			\
	},

#define LED_GPIO_DEVICE(i)						\
									\
DT_INST_FOREACH_CHILD(i, LED_GPIO_DT_SPEC)				\
									\
static const struct led_gpio led_gpio_##i[] = {				\
	DT_INST_FOREACH_CHILD(i, LED_GPIO)				\
};									\
									\
static const struct led_gpio_config led_gpio_config_##i = {		\
	.num_leds	= ARRAY_SIZE(led_gpio_##i),			\
	.led		= led_gpio_##i,					\
};									\
									\
DEVICE_DT_INST_DEFINE(i, &led_gpio_init, NULL,				\
		      NULL, &led_gpio_config_##i,			\
		      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,		\
		      &led_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(LED_GPIO_DEVICE)
