/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>

#define SKY_UFLn_GPIO_SPEC	GPIO_DT_SPEC_GET(DT_NODELABEL(sky13351), vctl1_gpios)
#define SKY_PCBn_GPIO_SPEC	GPIO_DT_SPEC_GET(DT_NODELABEL(sky13351), vctl2_gpios)

static inline void external_antenna(bool on)
{
	struct gpio_dt_spec ufl_gpio = SKY_UFLn_GPIO_SPEC;
	struct gpio_dt_spec pcb_gpio = SKY_PCBn_GPIO_SPEC;

	if (!device_is_ready(ufl_gpio.port)) {
		return;
	}

	if (!device_is_ready(pcb_gpio.port)) {
		return;
	}

	gpio_pin_configure_dt(&ufl_gpio, (on ? GPIO_OUTPUT_ACTIVE : GPIO_OUTPUT_INACTIVE));
	gpio_pin_configure_dt(&pcb_gpio, (on ? GPIO_OUTPUT_INACTIVE : GPIO_OUTPUT_ACTIVE));
}

static int board_particle_argon_init(void)
{

	/*
	 * On power-up the SKY13351 is left uncontrolled, so neither
	 * PCB nor external antenna is selected.  Select the PCB
	 * antenna.
	 */
	external_antenna(false);

	return 0;
}

/* needs to be done after GPIO driver init, which is at
 * POST_KERNEL:KERNEL_INIT_PRIORITY_DEFAULT.
 */
SYS_INIT(board_particle_argon_init, POST_KERNEL, 99);
