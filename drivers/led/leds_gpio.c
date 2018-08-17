/*
 * Copyright (c) 2018 Manivannan Sadhasivam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LEDs driver for GPIOs
 *
 * This driver controls LED devices attached to GPIO pins which
 * are declared under leds node in board devicetree. The driver
 * takes care of configuring the pin to output state before using it.
 *
 * TODO: Implement Software Blinking
 */

#include <device.h>
#include <gpio.h>
#include <led.h>
#include <misc/util.h>
#include <zephyr.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LED_LEVEL
#include <logging/sys_log.h>

#include "led_context.h"

enum led_gpio_modes {
	LED_OFF,
	LED_ON,
};

struct led_gpio_cfg {
	char *gpio_port;
	u8_t gpio_pin;
	u8_t gpio_polarity;
};

struct led_gpio_data {
	struct device *gpio;
	struct led_data dev_data;
};

static inline int led_gpio_on(struct device *dev, u32_t led)
{
	const struct led_gpio_cfg *cfg = dev->config->config_info;
	struct led_gpio_data *data = dev->driver_data;

	/* Make sure that the LED is valid */
	if (led != cfg->gpio_pin) {
		return -EINVAL;
	}

	/*
	 * Check for active high or active low defined in DT. For
	 * simplicity, non zero cases are considered as active high
	 * and zeroes are active low.
	 */
	gpio_pin_write(data->gpio, led,
		       (cfg->gpio_polarity & GPIO_INT_ACTIVE_HIGH) != 0);

	return 0;
}

static inline int led_gpio_off(struct device *dev, u32_t led)
{
	const struct led_gpio_cfg *cfg = dev->config->config_info;
	struct led_gpio_data *data = dev->driver_data;

	/* Make sure that the LED is valid */
	if (led != cfg->gpio_pin) {
		return -EINVAL;
	}

	/*
	 * Check for active high or active low defined in DT. For
	 * simplicity, non zero cases are considered as active high
	 * and zeroes are active low.
	 */
	gpio_pin_write(data->gpio, led,
		       (cfg->gpio_polarity & GPIO_INT_ACTIVE_HIGH) == 0);

	return 0;
}

static int led_gpio_init(struct device *dev)
{
	const struct led_gpio_cfg *cfg = dev->config->config_info;
	struct led_gpio_data *data = dev->driver_data;
	struct led_data *dev_data = &data->dev_data;

	data->gpio = device_get_binding(cfg->gpio_port);
	if (!data->gpio) {
		SYS_LOG_DBG("Failed to get GPIO device");
		return -EINVAL;
	}

	/* Set LED pin as output */
	gpio_pin_configure(data->gpio, cfg->gpio_pin, GPIO_DIR_OUT);

	/* Blinking and Brightness are not supported yet */
	dev_data->min_period = 0;
	dev_data->max_period = 0;
	dev_data->min_brightness = 0;
	dev_data->max_brightness = 0;

	return 0;
}

static const struct led_driver_api led_gpio_api = {
	.on = led_gpio_on,
	.off = led_gpio_off,
};

#define	DEFINE_LED_GPIO(_num)						\
									\
static struct led_gpio_data led_gpio_data_##_num;			\
									\
static const struct led_gpio_cfg led_gpio_cfg_##_num = {		\
	.gpio_port	= LED##_num##_GPIO_CONTROLLER,			\
	.gpio_pin	= LED##_num##_GPIO_PIN,				\
	.gpio_polarity	= LED##_num##_GPIO_FLAGS,			\
};									\
									\
DEVICE_AND_API_INIT(led_gpio_##_num, LED##_num##_LABEL,			\
	    led_gpio_init,						\
	    &led_gpio_data_##_num,					\
	    &led_gpio_cfg_##_num,					\
	    POST_KERNEL, CONFIG_LED_INIT_PRIORITY,			\
	    &led_gpio_api)

#ifdef CONFIG_LED_GPIO_0
DEFINE_LED_GPIO(0);
#endif

#ifdef CONFIG_LED_GPIO_1
DEFINE_LED_GPIO(1);
#endif
