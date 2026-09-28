/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: Luciano Carricart <carricartluciano@gmail.com> (TM4C129 support)
 * Based on TM4C123 support by Sri Surya <srisurya@linumiz.com>
 */

/*
 * SoC initialization for the TI Tiva C TM4C129x Series
 *
 * Configures the PLL to run at 120 MHz from the 25 MHz main oscillator.
 */

#include <zephyr/init.h>

#include "soc.h"

void soc_early_init_hook(void)
{
	/* Configure PLL: 120 MHz from 25 MHz crystal */
	SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
				   SYSCTL_CFG_VCO_480,
			   120000000);
}
