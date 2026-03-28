/*
 * Copyright (c) 2019 Foundries.io
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>

#define ANT_UFLn_GPIO_SPEC	GPIO_DT_SPEC_GET(DT_NODELABEL(sky13351), vctl1_gpios)

static inline void external_antenna(bool on)
{
	struct gpio_dt_spec ufl_gpio = ANT_UFLn_GPIO_SPEC;

	/*
	 * On power-up the SKY13351 is left uncontrolled, so neither
	 * PCB nor external antenna is selected.  Select the PCB
	 * antenna.
	 */
	if (!gpio_is_ready_dt(&ufl_gpio)) {
		return;
	}

	gpio_pin_configure_dt(&ufl_gpio, (on ? GPIO_OUTPUT_ACTIVE : GPIO_OUTPUT_INACTIVE));
}

static int board_particle_boron_init(void)
{
	external_antenna(false);

	return 0;
}

SYS_INIT(board_particle_boron_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
