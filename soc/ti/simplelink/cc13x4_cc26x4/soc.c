/*
 * Copyright (c) 2026 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <driverlib/setup.h>

void soc_early_init_hook()
{
	/* Perform necessary trim of the device. */
	SetupTrimDevice();
}
