/*
 * Copyright (c) 2019 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/led.h>
#include <drivers/gpio.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

#define LED_DEV_NAME DT_INST_0_HOLTEK_HT16K33_LABEL
#define KS0_DEV_NAME DT_INST_0_HOLTEK_HT16K33_KEYSCAN_LABEL
#define KS1_DEV_NAME DT_INST_1_HOLTEK_HT16K33_KEYSCAN_LABEL
#define KS2_DEV_NAME DT_INST_2_HOLTEK_HT16K33_KEYSCAN_LABEL

#define KEYSCAN_DEVICES 3

struct device *led_dev;
struct device *ks_dev[KEYSCAN_DEVICES];
static struct gpio_callback ks_cb[KEYSCAN_DEVICES];

static void keyscan_callback(struct device *gpiob,
			     struct gpio_callback *cb, u32_t pins)
{
	LOG_INF("%s: 0x%08x", gpiob->config->name, pins);
}

void main(void)
{
	int err;
	int i;

	/* LED device binding */
	led_dev = device_get_binding(LED_DEV_NAME);
	if (!led_dev) {
		LOG_ERR("LED device %s not found", LED_DEV_NAME);
		return;
	}

	/* Keyscan device bindings */
	ks_dev[0] = device_get_binding(KS0_DEV_NAME);
	ks_dev[1] = device_get_binding(KS1_DEV_NAME);
	ks_dev[2] = device_get_binding(KS2_DEV_NAME);
	for (i = 0; i < ARRAY_SIZE(ks_dev); i++) {
		if (!ks_dev[i]) {
			LOG_ERR("KS%d device not found", i);
			return;
		}

		gpio_init_callback(&ks_cb[i], &keyscan_callback,
				   GENMASK(12, 0));

		err = gpio_add_callback(ks_dev[i], &ks_cb[i]);
		if (err) {
			LOG_ERR("Failed to add KS%d GPIO callback (err %d)", i,
				err);
			return;
		}
	}

	while (1) {
		LOG_INF("Iterating through all LEDs, turning them on "
			"one-by-one");
		for (i = 0; i < 128; i++) {
			led_on(led_dev, i);
			k_sleep(K_MSEC(100));
		}

		for (i = 500; i <= 2000; i *= 2) {
			LOG_INF("Blinking LEDs with a period of %d ms", i);
			led_blink(led_dev, 0, i / 2, i / 2);
			k_sleep(10 * i);
		}
		led_blink(led_dev, 0, 0, 0);

		for (i = 100; i >= 0; i -= 10) {
			LOG_INF("Setting LED brightness to %d%%", i);
			led_set_brightness(led_dev, 0, i);
			k_sleep(K_MSEC(1000));
		}

		LOG_INF("Turning all LEDs off and restoring 100%% brightness");
		for (i = 0; i < 128; i++) {
			led_off(led_dev, i);
		}
		led_set_brightness(led_dev, 0, 100);
	}
}
