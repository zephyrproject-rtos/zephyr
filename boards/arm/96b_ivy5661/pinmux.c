/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include "pinmux.h"
#include "uwp_hal.h"

int pinmux_initialize(struct device *port)
{
	ARG_UNUSED(port);

	return 0;
}

SYS_INIT(pinmux_initialize, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
