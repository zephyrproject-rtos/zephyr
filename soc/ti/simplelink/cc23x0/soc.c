/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <driverlib/setup.h>

static int ti_cc23x0_init(void)
{
	/* Perform necessary trim of the device. */
	SetupTrimDevice();

	return 0;
}

/* Call initialisation function as early as possible */
SYS_INIT(ti_cc23x0_init, EARLY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
