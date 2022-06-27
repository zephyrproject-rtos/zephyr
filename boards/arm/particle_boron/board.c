/*
 * Copyright (c) 2019 Foundries.io
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include "board.h"

static inline void external_antenna(bool on)
{
	const struct device *gpio_dev;

	/*
	 * On power-up the SKY13351 is left uncontrolled, so neither
	 * PCB nor external antenna is selected.  Select the PCB
	 * antenna.
	 */
	gpio_dev = device_get_binding(ANT_UFLn_GPIO_NAME);
	if (!gpio_dev) {
		return;
	}

	gpio_pin_configure(gpio_dev, ANT_UFLn_GPIO_PIN,
			   ANT_UFLn_GPIO_FLAGS
			   | (on
			      ? GPIO_OUTPUT_ACTIVE
			      : GPIO_OUTPUT_INACTIVE));
}

static int board_particle_boron_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	external_antenna(false);

#if defined(CONFIG_MODEM_UBLOX_SARA)
	const struct device *gpio_dev;

	/* Enable the serial buffer for SARA-R4 modem */
	gpio_dev = device_get_binding(SERIAL_BUFFER_ENABLE_GPIO_NAME);
	if (!gpio_dev) {
		return -ENODEV;
	}

	gpio_pin_configure(gpio_dev, V_INT_DETECT_GPIO_PIN,
			   GPIO_INPUT | V_INT_DETECT_GPIO_FLAGS);

	gpio_pin_configure(gpio_dev, SERIAL_BUFFER_ENABLE_GPIO_PIN,
			   GPIO_OUTPUT_ACTIVE
			   | SERIAL_BUFFER_ENABLE_GPIO_FLAGS);
#endif

	return 0;
}

/* needs to be done after GPIO driver init, which is at
 * POST_KERNEL:KERNEL_INIT_PRIORITY_DEFAULT.
 */
SYS_INIT(board_particle_boron_init, POST_KERNEL, 99);
