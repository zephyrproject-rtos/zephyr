/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#include <driverlib/setup.h>

void soc_early_init_hook(void)
{

	/* Performs necessary trim of the device. */
	SetupTrimDevice();
}
