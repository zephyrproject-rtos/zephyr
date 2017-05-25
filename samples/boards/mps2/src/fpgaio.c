/*
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gpio.h>
#include <misc/printk.h>
#include <soc.h>

int test_fpgaio(void)
{
	struct device *buttons =  device_get_binding(FPGAIO_BUTTON_GPIO_NAME);
	struct device *leds =  device_get_binding(FPGAIO_LED0_GPIO_NAME);
	int button[2];

	if (!buttons || !leds) {
		return -ENODEV;
	}
	printk("Press buttons to light LEDs, press both together to end\n");

	do {
		int ret;

		ret = gpio_pin_read(buttons, 0, &button[0]);
		if (ret) {
			return ret;
		}
		ret = gpio_pin_write(leds, 0, button[0]);
		if (ret) {
			return ret;
		}

		ret = gpio_pin_read(buttons, 1, &button[1]);
		if (ret) {
			return ret;
		}
		ret = gpio_pin_write(leds, 1, button[1]);
		if (ret) {
			return ret;
		}
	} while (!(button[0] && button[1]));

	return 0;
}
