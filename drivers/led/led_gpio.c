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

#include "led_blink.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_gpio, CONFIG_LED_LOG_LEVEL);

struct led_gpio_config {
	size_t num_leds;
	const struct gpio_dt_spec *led;
};

struct led_gpio_data {
#ifdef CONFIG_LED_BLINK_SOFTWARE
	struct led_blink_software_data *blink_data;
#endif
};

#ifdef CONFIG_LED_BLINK_SOFTWARE
struct led_blink_software_data *led_gpio_blink_data(const struct device *dev, uint32_t led)
{
	const struct led_gpio_config *config = dev->config;
	struct led_gpio_data *data = dev->data;

	if (led >= config->num_leds) {
		return NULL;
	}

	return &data->blink_data[led];
}
#endif

static int led_gpio_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct led_gpio_config *config = dev->config;
	const struct gpio_dt_spec *led_gpio;

	if (led >= config->num_leds) {
		return -EINVAL;
	}

	led_gpio = &config->led[led];

	return gpio_pin_set_dt(led_gpio, value > 0);
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
		const struct gpio_dt_spec *led = &config->led[i];

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

	return err;
}

static DEVICE_API(led, led_gpio_api) = {
	.set_brightness	= led_gpio_set_brightness,
#ifdef CONFIG_LED_BLINK_SOFTWARE
	.get_blink_data	= led_gpio_blink_data,
#endif
};

#define LED_GPIO_DEVICE(i)					\
								\
static const struct gpio_dt_spec gpio_dt_spec_##i[] = {		\
	DT_INST_FOREACH_CHILD_SEP_VARGS(i, GPIO_DT_SPEC_GET, (,), gpios) \
};								\
								\
static const struct led_gpio_config led_gpio_config_##i = {	\
	.num_leds	= ARRAY_SIZE(gpio_dt_spec_##i),		\
	.led		= gpio_dt_spec_##i,			\
};								\
								\
static struct led_gpio_data led_gpio_data_##i = {		\
	LED_BLINK_SOFTWARE_DATA(i, blink_data)			\
};								\
								\
DEVICE_DT_INST_DEFINE(i, &led_gpio_init, NULL,			\
				 (&led_gpio_data_##i),		\
		      &led_gpio_config_##i,			\
		      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,	\
		      &led_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(LED_GPIO_DEVICE)
