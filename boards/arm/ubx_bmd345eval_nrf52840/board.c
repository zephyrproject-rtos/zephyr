/*
 * Copyright (c) 2021 u-blox AG
 * Copyright (c) 2021 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define MODE_PIN	4 /* P1.04 */
#define A_SEL_PIN	2 /* P1.02 */

static int bmd345_fem_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	int ret;
	const struct device *mode_asel_port_dev;

	mode_asel_port_dev = DEVICE_DT_GET(DT_NODELABEL(gpio1));

	if (!device_is_ready(mode_asel_port_dev)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure(mode_asel_port_dev, MODE_PIN, GPIO_OUTPUT_LOW);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure(mode_asel_port_dev, A_SEL_PIN, GPIO_OUTPUT_LOW);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

SYS_INIT(bmd345_fem_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
