/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/gpio.h>

#define SKY_UFLn_GPIO_SPEC	GPIO_DT_SPEC_GET(DT_NODELABEL(sky13351), vctl1_gpios)
#define SKY_PCBn_GPIO_SPEC	GPIO_DT_SPEC_GET(DT_NODELABEL(sky13351), vctl2_gpios)

static inline void external_antenna(bool on)
{
	struct gpio_dt_spec ufl_gpio = SKY_UFLn_GPIO_SPEC;
	struct gpio_dt_spec pcb_gpio = SKY_PCBn_GPIO_SPEC;

	if (!gpio_is_ready_dt(&ufl_gpio)) {
		return;
	}

	if (!gpio_is_ready_dt(&pcb_gpio)) {
		return;
	}

	gpio_pin_configure_dt(&ufl_gpio, (on ? GPIO_OUTPUT_ACTIVE : GPIO_OUTPUT_INACTIVE));
	gpio_pin_configure_dt(&pcb_gpio, (on ? GPIO_OUTPUT_INACTIVE : GPIO_OUTPUT_ACTIVE));
}

void board_late_init_hook(void)
{

	/*
	 * On power-up the SKY13351 is left uncontrolled, so neither
	 * PCB nor external antenna is selected.  Select the PCB
	 * antenna.
	 */
	external_antenna(false);
}
