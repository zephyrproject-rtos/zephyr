/*
 * Copyright (c) 2016, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/prcm.h>

static int ti_cc3200_init(struct device *arg)
{
	ARG_UNUSED(arg);

	PRCMCC3200MCUInit();

#ifdef CONFIG_UART_CC32XX
	/*
	 * Enable Peripheral Clocks, ensuring UART can wake the processor from
	 * idle (after ARM wfi instruction)
	 */
	MAP_PRCMPeripheralClkEnable(PRCM_UARTA0, PRCM_RUN_MODE_CLK |
				    PRCM_SLP_MODE_CLK);
#endif

	return 0;
}

SYS_INIT(ti_cc3200_init, PRE_KERNEL_1, 0);
