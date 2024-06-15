/*
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define NUM_LEDS             3
#define BLINK_DELAY_ON       500
#define BLINK_DELAY_OFF      500
#define DELAY_TIME           2000
#define COLORS_TO_SHOW       8
#define VALUES_PER_COLOR     3
#define DELAY_TIME_ON        K_MSEC(1000)
#define DELAY_TIME_BREATHING K_MSEC(20)

/*
 * We assume that the colors are connected the way it is described in the driver
 * datasheet.
 */

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(ti_lp5521);
	int ret;
	uint8_t i;

	if (!dev) {
		LOG_ERR("No \"ti,lp5521\" device found");
		return 0;
	} else if (!device_is_ready(dev)) {
		LOG_ERR("LED device %s is not ready", dev->name);
		return 0;
	}

	for (i = 0; i < NUM_LEDS; i++) {
		ret = led_off(dev, i);
		if (ret) {
			return ret;
		}
	}
	LOG_INF("Testing leds");

	/*
	 * Display a continuous pattern that turns on 3 LEDs at 1 s one by
	 * one until it reaches the end and turns off LEDs in reverse order.
	 */

	while (1) {

		/* Turn on LEDs one by one */
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_on(dev, i);
			if (ret) {
				return ret;
			}

			k_sleep(DELAY_TIME_ON);
		}

		/* Turn all LEDs off slowly to demonstrate set_brightness */
		for (i = 0; i <= 100; i++) {
			for (int j = 0; j < NUM_LEDS; j++) {
				ret = led_set_brightness(dev, j, 100 - i);
				if (ret) {
					return ret;
				}
			}
			k_sleep(DELAY_TIME_BREATHING);
		}
	}
	return 0;
}
