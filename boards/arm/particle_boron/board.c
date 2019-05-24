/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include "board.h"
#include <gpio.h>

static int particle_boron_init(struct device *dev)
{

	ARG_UNUSED(dev);

#if defined(CONFIG_MODEM_UBLOX_SARA_R4)
	struct device *gpio_dev;

	/* Enable the serial buffer for SARA-R4 modem */
	gpio_dev = device_get_binding(SERIAL_BUFFER_ENABLE_GPIO_NAME);
	if (!gpio_dev) {
		return -ENODEV;
	}

	gpio_pin_configure(gpio_dev, V_INT_DETECT_GPIO_PIN, GPIO_DIR_IN);

	gpio_pin_configure(gpio_dev, SERIAL_BUFFER_ENABLE_GPIO_PIN,
			   GPIO_DIR_OUT);
#endif

	return 0;
}

/* needs to be done after GPIO driver init */
SYS_INIT(particle_boron_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
