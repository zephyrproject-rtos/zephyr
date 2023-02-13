/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#include <driverlib/setup.h>

static int ti_cc13x2_cc26x2_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Performs necessary trim of the device. */
	SetupTrimDevice();

	return 0;
}

SYS_INIT(ti_cc13x2_cc26x2_init, PRE_KERNEL_1, 0);
