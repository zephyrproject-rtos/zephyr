/*
 * Copyright (c) 2022 Vaishnav Achath
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
