/*
 * Copyright (c) 2019 Foundries.io
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/gpio.h>
#include "board.h"

static inline void external_antenna(bool on)
{
	struct device *ant_sel_gpio_dev;

	/*
	 * On power-up the SKY13351 is left uncontrolled, so neither
	 * PCB nor external antenna is selected.  Select the PCB
	 * antenna.
	 */
	ant_sel_gpio_dev = device_get_binding(ANT_SEL_GPIO_NAME);
	if (!ant_sel_gpio_dev) {
		return;
	}

	gpio_pin_configure(ant_sel_gpio_dev, ANT_SEL_GPIO_PIN,
			   GPIO_DIR_OUT | ANT_SEL_GPIO_FLAGS);

	if (on) {
		gpio_pin_write(ant_sel_gpio_dev, ANT_SEL_GPIO_PIN, 1);
	} else {
		gpio_pin_write(ant_sel_gpio_dev, ANT_SEL_GPIO_PIN, 0);
	}
}

static int board_particle_boron_init(struct device *dev)
{
	ARG_UNUSED(dev);

	external_antenna(false);

#if defined(CONFIG_MODEM_UBLOX_SARA)
	struct device *gpio_dev;

	/* Enable the serial buffer for SARA-R4 modem */
	gpio_dev = device_get_binding(SERIAL_BUFFER_ENABLE_GPIO_NAME);
	if (!gpio_dev) {
		return -ENODEV;
	}

	gpio_pin_configure(gpio_dev, V_INT_DETECT_GPIO_PIN, GPIO_DIR_IN);

	gpio_pin_configure(gpio_dev, SERIAL_BUFFER_ENABLE_GPIO_PIN,
			   GPIO_DIR_OUT);
	gpio_pin_write(gpio_dev, SERIAL_BUFFER_ENABLE_GPIO_PIN, 0);
#endif

	return 0;
}

/* needs to be done after GPIO driver init */
SYS_INIT(board_particle_boron_init, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
