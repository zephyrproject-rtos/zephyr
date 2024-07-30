/*
 * Copyright (c) 2023 Endor AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define MAX_BRIGHTNESS	100
#define LAST_LED	15

#define SLEEP_DELAY_MS			100
#define FADE_DELAY_MS			10
#define WRITE_CHANNELS_LEDS_COUNT	3
#define WRITE_CHANNELS_LED_START	2

static void pulse_led(int led, const struct device *const dev)
{
	int status;
	uint8_t percent;

	for (percent = 1 ; percent <= MAX_BRIGHTNESS ; percent++) {
		status = led_set_brightness(dev, led, percent);
		if (status) {
			LOG_ERR("Could not change brightness: %i", status);
			return;
		}
		k_msleep(FADE_DELAY_MS);
	}
	k_msleep(SLEEP_DELAY_MS);
	for (percent = MAX_BRIGHTNESS;
	     percent <= MAX_BRIGHTNESS; percent--) {
		status = led_set_brightness(dev, led, percent);
		if (status) {
			LOG_ERR("Could not change brightness: %i", status);
			return;
		}
		k_msleep(FADE_DELAY_MS);
	}
}

static void pulse_leds(const struct device *const dev)
{
	int status;
	uint8_t brightness[WRITE_CHANNELS_LEDS_COUNT];
	uint8_t percent;

	for (percent = 1; percent <= MAX_BRIGHTNESS; percent++) {
		memset(brightness, percent, sizeof(brightness));
		status = led_write_channels(dev, WRITE_CHANNELS_LED_START,
					    WRITE_CHANNELS_LEDS_COUNT,
					    brightness);
		if (status) {
			LOG_ERR("Could not change brightness: %i", status);
			return;
		}
		k_msleep(FADE_DELAY_MS);
	}
	k_msleep(SLEEP_DELAY_MS);
	for (percent = MAX_BRIGHTNESS;
	     percent <= MAX_BRIGHTNESS; percent--) {
		memset(brightness, percent, sizeof(brightness));
		status = led_write_channels(dev, WRITE_CHANNELS_LED_START,
					    WRITE_CHANNELS_LEDS_COUNT,
					    brightness);
		if (status) {
			LOG_ERR("Could not change brightness: %i", status);
			return;
		}
		k_msleep(FADE_DELAY_MS);
	}
}

int main(void)
{
	int led;
	const struct device *const is31fl3216a =
		DEVICE_DT_GET_ANY(issi_is31fl3216a);

	if (!is31fl3216a) {
		LOG_ERR("No device with compatible issi,is31fl3216a found");
		return 0;
	} else if (!device_is_ready(is31fl3216a)) {
		LOG_ERR("LED controller %s is not ready", is31fl3216a->name);
		return 0;
	}

	LOG_INF("Found LED controller %s", is31fl3216a->name);
	for (;;) {
		LOG_INF("Pulsing single LED");
		for (led = 0 ; led <= LAST_LED ; led++) {
			pulse_led(led, is31fl3216a);
		}
		LOG_INF("Pulsing multiple LEDs");
		pulse_leds(is31fl3216a);
	}

	return 0;
}
