/*
 * Copyright (c) 2024 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <ti/driverlib/m0p/dl_core.h>
#include <soc.h>

static int ti_mspm0g_init(void)
{
	/* Reset and enable GPIO banks */
	DL_GPIO_reset(GPIOA);
	DL_GPIO_reset(GPIOB);

	DL_GPIO_enablePower(GPIOA);
	DL_GPIO_enablePower(GPIOB);

	/* Allow delay time to settle */
	delay_cycles(POWER_STARTUP_DELAY);

	DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);

	/* Low Power Mode is configured to be SLEEP0 */
	DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

	return 0;
}

SYS_INIT(ti_mspm0g_init, PRE_KERNEL_1, 0);
