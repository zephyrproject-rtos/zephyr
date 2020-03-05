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

void led_on_off(bool led_state)
{
	int led_param = (led_state == true) ? 1 : 0;
	if (led_dev) {
		gpio_pin_set(led_dev, LED, led_param);
	}
}

int led_init(void)
{
	int ret;

	led_dev = device_get_binding(LED_PORT);
	if (!led_dev) {
		return (-EOPNOTSUPP);
	}

	/* Set LED pin as output */

	ret = gpio_pin_configure(led_dev, LED,
				 GPIO_OUTPUT_ACTIVE
				 | DT_ALIAS_LED0_GPIOS_FLAGS);
	if (ret < 0) {
		LOG_ERR("Error %d: failed to configure pin %d '%s'\n",
		ret, LED, DT_ALIAS_LED0_LABEL);
		return ret;
	}

	led_state = false;
	led_on_off(led_state);
	return 0;
}
