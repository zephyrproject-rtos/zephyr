/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>

#define SKY_UFLn_PIN 7

static inline void external_antenna(bool on)
{
	if (on) {
		NRF_P0->OUTCLR = (1U << SKY_UFLn_PIN);
	} else {
		NRF_P0->OUTSET = (1U << SKY_UFLn_PIN);
	}
}

static int board_particle_boron_init(struct device *dev)
{
	ARG_UNUSED(dev);

	/*
	 * On power-up the SKY13351 is left uncontrolled, so neither
	 * PCB nor external antenna is selected.  Select the PCB
	 * antenna.
	 */
	NRF_P0->PIN_CNF[SKY_UFLn_PIN] =
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
		(GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
	external_antenna(false);

	return 0;
}

SYS_INIT(board_particle_boron_init, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
