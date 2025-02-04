/*
 * Copyright (c) 2024 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <soc.h>

void soc_early_init_hook(void)
{
	/* Reset and enable GPIO banks */
	DL_GPIO_reset(GPIOA);
	DL_GPIO_reset(GPIOB);

	DL_GPIO_enablePower(GPIOA);
	DL_GPIO_enablePower(GPIOB);

	/* Allow delay time to settle */
	delay_cycles(POWER_STARTUP_DELAY);

	/* Low Power Mode is configured to be SLEEP0 */
	DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);
}
