/** @file
 *  @brief Button Service sample
 */

/*
 * Copyright (c) 2019 Marcio Montenegro <mtuxpe@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>
#include <drivers/gpio.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(led_svc);

#define LED_PORT DT_ALIAS_LED0_GPIOS_CONTROLLER
#define LED     DT_ALIAS_LED0_GPIOS_PIN

struct device *led_dev;
bool led_state;

void led_on_off(u32_t led_state)
{
	if (led_dev) {
		gpio_pin_write(led_dev, LED, led_state);
	}
}

int led_init(void)
{
	led_dev = device_get_binding(LED_PORT);
	if (!led_dev) {
		return (-EOPNOTSUPP);
	}

	/* Set LED pin as output */
	gpio_pin_configure(led_dev, LED, GPIO_DIR_OUT);
	led_state = false;
	gpio_pin_write(led_dev, LED, 0);
	led_on_off(0);
	return 0;
}
