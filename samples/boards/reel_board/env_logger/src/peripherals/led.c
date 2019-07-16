/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <gpio.h>
#include <logging/log.h>

#include "led.h"

LOG_MODULE_REGISTER(app_led, LOG_LEVEL_DBG);

struct k_delayed_work led_init_timer;

struct led_device_info {
	struct device *dev;
	char *name;
	u32_t pin;
} leds[] = {
	{
		.name = DT_ALIAS_LED0_GPIOS_CONTROLLER,
		.pin = DT_ALIAS_LED0_GPIOS_PIN,
	},
	{
		.name = DT_ALIAS_LED1_GPIOS_CONTROLLER,
		.pin = DT_ALIAS_LED1_GPIOS_PIN,
	},
	{
		.name = DT_ALIAS_LED2_GPIOS_CONTROLLER,
		.pin = DT_ALIAS_LED2_GPIOS_PIN,
	},
	{
		.name = DT_ALIAS_LED3_GPIOS_CONTROLLER,
		.pin = DT_ALIAS_LED3_GPIOS_PIN,
	}
};

static void led_init_timeout(struct k_work *work)
{
	/* Disable all LEDs */
	for (u8_t i = 0; i < ARRAY_SIZE(leds); i++) {
		gpio_pin_write(leds[i].dev, leds[i].pin, 1);
	}
}

int led_init(void)
{
	for (u8_t i = 0; i < ARRAY_SIZE(leds); i++) {
		leds[i].dev = device_get_binding(leds[i].name);
		if (!leds[i].dev) {
			LOG_ERR("Failed to get %s device", leds[i].name);
			return -ENODEV;
		}

		gpio_pin_configure(leds[i].dev, leds[i].pin, GPIO_DIR_OUT);
		gpio_pin_write(leds[i].dev, leds[i].pin, 1);
	}

	k_delayed_work_init(&led_init_timer, led_init_timeout);
	return 0;
}

int led_set(enum led_idx idx, bool status)
{
	if (idx >= LED_MAX) {
		return -ENODEV;
	}

	u32_t value = status ? 0 : 1;

	gpio_pin_write(leds[idx].dev, leds[idx].pin, value);
	LOG_DBG("LED[%d] status changed to: %s", idx, value ? "on" : "off");

	return 0;
}

int led_set_time(enum led_idx idx, size_t ms)
{
	int ret = led_set(idx, true);

	if (ret == 0) {
		k_delayed_work_submit(&led_init_timer, K_MSEC(ms));
	}

	return ret;
}

