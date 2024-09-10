/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#include <am_mcu_apollo.h>

extern void ambiq_power_init(void);
void soc_early_init_hook(void)
{

	/* Initialize for low power in the power control block */
	am_hal_pwrctrl_low_power_init();

	/* Enable SIMOBUCK for the Apollo4 Family */
	am_hal_pwrctrl_control(AM_HAL_PWRCTRL_CONTROL_SIMOBUCK_INIT, 0);

	/* Disable the RTC. */
	am_hal_rtc_osc_disable();
#ifdef CONFIG_PM
	ambiq_power_init();
#endif
}
