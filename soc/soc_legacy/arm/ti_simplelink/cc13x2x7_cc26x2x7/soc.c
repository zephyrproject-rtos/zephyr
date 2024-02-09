/*
 * Copyright (c) 2022 Vaishnav Achath
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#include <driverlib/setup.h>

static int ti_cc13x7_cc26x7_init(void)
{

	/* Performs necessary trim of the device. */
	SetupTrimDevice();

	return 0;
}

SYS_INIT(ti_cc13x7_cc26x7_init, PRE_KERNEL_1, 0);
