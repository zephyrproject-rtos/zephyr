/*
 * Copyright (c) 2026 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <driverlib/setup.h>

static int ti_cc13x4_cc26x4_init(void)
{
	/* Perform necessary trim of the device. */
	SetupTrimDevice();
	/*values for the analog peripherals from FCFG*/

	return 0;
}

/* Call initialisation function as early as possible */
SYS_INIT(ti_cc13x4_cc26x4_init, EARLY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
