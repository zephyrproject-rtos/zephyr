/*
 * Copyright (c) 2023 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_LED_LOG_LEVEL);

#define NUM_LEDS 9
#define DELAY_TIME_ON		K_MSEC(1000)
#define DELAY_TIME_BREATHING	K_MSEC(20)

int main(void)
{
	const struct device *const led_dev = DEVICE_DT_GET_ANY(ti_lp5569);
	uint8_t ch_buf[9] = {0};
	int i, ret;

	if (!led_dev) {
		LOG_ERR("No device with compatible ti,lp5569 found");
		return 0;
	} else if (!device_is_ready(led_dev)) {
		LOG_ERR("LED device %s not ready", led_dev->name);
		return 0;
	}

	LOG_INF("Found LED device %s", led_dev->name);

	/*
	 * Display a continuous pattern that turns on 9 LEDs at 1 s one by
	 * one until it reaches the end and fades off all LEDs.
	 * Afterwards, all LEDs get blinked once at the same time.
	 */

	LOG_INF("Testing 9 LEDs ..");

	while (1) {
		/* Turn on LEDs one by one */
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_on(led_dev, i);
			if (ret) {
				return ret;
			}

			k_sleep(DELAY_TIME_ON);
		}

		/* Turn all LEDs off slowly to demonstrate set_brightness */
		for (i = 0; i <= 100; i++) {
			for (int j = 0; j < NUM_LEDS; j++) {
				ret = led_set_brightness(led_dev, j, 100 - i);
				if (ret) {
					return ret;
				}
			}

			k_sleep(DELAY_TIME_BREATHING);
		}

		k_sleep(DELAY_TIME_ON);

		/* Turn on all LEDs at once to demonstrate write_channels */
		memset(ch_buf, 255, ARRAY_SIZE(ch_buf));
		ret = led_write_channels(led_dev, 0, ARRAY_SIZE(ch_buf), ch_buf);
		if (ret) {
			return ret;
		}

		k_sleep(DELAY_TIME_ON);

		/* Turn off all LEDs at once to demonstrate write_channels */
		memset(ch_buf, 0, ARRAY_SIZE(ch_buf));
		ret = led_write_channels(led_dev, 0, ARRAY_SIZE(ch_buf), ch_buf);
		if (ret) {
			return ret;
		}

		k_sleep(DELAY_TIME_ON);
	}
	return 0;
}
