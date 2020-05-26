/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_leds

/**
 * @file
 * @brief LED driver for GPIO connected LEDs.
 */

#include <drivers/gpio.h>
#include <drivers/led.h>
#include <device.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_LED_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(led_gpio);

#define DEV_CFG(dev)  ((const struct led_gpio_config *) \
		       ((dev)->config_info))
#define DEV_DATA(dev) ((struct led_gpio_data *) \
		       ((dev)->driver_data))

struct led_gpio_config {
	char *gpio_controller_label;
	u32_t gpio_pin;
	u32_t gpio_flags;
};

struct led_gpio_data {
	struct device *gpio_dev;
};

static int led_gpio_set_brightness(struct device *dev, u32_t led, u8_t value)
{
	const struct led_gpio_config *config = DEV_CFG(dev);
	struct led_gpio_data *data = DEV_DATA(dev);

	if (led != 0) {
		return -EINVAL;
	}

	return gpio_pin_set(data->gpio_dev, config->gpio_pin, value);
}

static int led_gpio_on(struct device *dev, u32_t led)
{
	return led_gpio_set_brightness(dev, led, 1);
}

static int led_gpio_off(struct device *dev, u32_t led)
{
	return led_gpio_set_brightness(dev, led, 0);
}

static int led_gpio_init(struct device *dev)
{
	const struct led_gpio_config *config = DEV_CFG(dev);
	struct led_gpio_data *data = DEV_DATA(dev);
	int err;

	data->gpio_dev = device_get_binding(config->gpio_controller_label);
	if (data->gpio_dev == NULL) {
		LOG_ERR("%s: device %s not found",
			dev->name, config->gpio_controller_label);
		return -ENODEV;
	}

	err = gpio_pin_configure(data->gpio_dev,
				 config->gpio_pin, config->gpio_flags);
	if (err != 0) {
		LOG_ERR("%s: failed to configure GPIO %s %d (error: %d)",
			dev->name, config->gpio_controller_label,
			config->gpio_pin, err);
	}

	return err;
}

static const struct led_driver_api led_gpio_api = {
	.set_brightness	= led_gpio_set_brightness,
	.on		= led_gpio_on,
	.off		= led_gpio_off,
};

#define LED_GPIO_DEVICE(id)					\
								\
static struct led_gpio_config led_gpio_config_##id = {		\
	.gpio_controller_label = DT_GPIO_LABEL(id, gpios),	\
	.gpio_pin = DT_GPIO_PIN(id, gpios),			\
	.gpio_flags = DT_GPIO_FLAGS(id, gpios)			\
			| GPIO_OUTPUT_INACTIVE,			\
};								\
								\
static struct led_gpio_data led_gpio_data_##id;			\
								\
DEVICE_AND_API_INIT(led_gpio_led_##id, DT_LABEL(id),		\
		    &led_gpio_init,				\
		    &led_gpio_data_##id,			\
		    &led_gpio_config_##id,			\
		    POST_KERNEL, CONFIG_LED_INIT_PRIORITY,	\
		    &led_gpio_api);

#define LED_GPIO_INST(inst) DT_INST_FOREACH_CHILD(inst, LED_GPIO_DEVICE)

DT_INST_FOREACH_STATUS_OKAY(LED_GPIO_INST)
