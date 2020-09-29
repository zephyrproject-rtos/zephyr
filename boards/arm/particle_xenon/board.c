/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/gpio.h>
#include "board.h"

static inline void external_antenna(bool on)
{
	const struct device *ufl_gpio_dev, *pcb_gpio_dev;

	ufl_gpio_dev = device_get_binding(SKY_UFLn_GPIO_NAME);
	if (!ufl_gpio_dev) {
		return;
	}

	pcb_gpio_dev = device_get_binding(SKY_PCBn_GPIO_NAME);
	if (!pcb_gpio_dev) {
		return;
	}

	gpio_pin_configure(ufl_gpio_dev, SKY_UFLn_GPIO_PIN,
			   SKY_UFLn_GPIO_FLAGS
			   | (on
			      ? GPIO_OUTPUT_ACTIVE
			      : GPIO_OUTPUT_INACTIVE));
	gpio_pin_configure(pcb_gpio_dev, SKY_PCBn_GPIO_PIN,
			   SKY_PCBn_GPIO_FLAGS
			   | (on
			      ? GPIO_OUTPUT_INACTIVE
			      : GPIO_OUTPUT_ACTIVE));
}

static int board_particle_xenon_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/*
	 * On power-up the SKY13351 is left uncontrolled, so neither
	 * PCB nor external antenna is selected.  Select the PCB
	 * antenna.
	 */
	external_antenna(false);

	return 0;
}

/* needs to be done after GPIO driver init */
SYS_INIT(board_particle_xenon_init, POST_KERNEL,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
