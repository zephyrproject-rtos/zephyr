/*
 * Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <driverlib/setup.h>

static int ti_cc35xx_init(void)
{
	/* Perform necessary trim of the device. */
	SetupTrimDevice();

	return 0;
}

/* Call initialization function as early as possible */
SYS_INIT(ti_cc35xx_init, EARLY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
