/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 Linumiz
 * Author: Sri Surya <srisurya@linumiz.com>
 */

/*
 * SoC initialization for the TI Tiva C TM4C123x Series
 *
 * Configures the PLL to run at 80 MHz from the 16 MHz main oscillator.
 */

#include <zephyr/init.h>

#include "soc.h"

void soc_early_init_hook(void)
{
	/* Configure PLL: 80 MHz from 16 MHz crystal */
	SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL |
			SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
}
