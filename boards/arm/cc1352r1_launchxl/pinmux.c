/*
 * Copyright 2020, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <drivers/gpio.h>

#include <driverlib/gpio.h>

#if defined(CONFIG_BT_CTLR_DEBUG_PINS)
struct device *ti_ble_debug_port;
#endif /* defined(CONFIG_BT_CTLR_DEBUG_PINS)  */

static int cc1352r1_launchxl_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

SYS_INIT(cc1352r1_launchxl_pinmux_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_INIT_PRIORITY);
