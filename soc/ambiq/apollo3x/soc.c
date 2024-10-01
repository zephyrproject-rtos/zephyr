/*
 * Copyright (c) 2023 Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#include <am_mcu_apollo.h>

extern void ambiq_power_init(void);

void soc_early_init_hook(void)
{
	/* Set the clock frequency. */
	am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);

	/* Enable Flash cache.*/
	am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
	am_hal_cachectrl_enable();

	/* Initialize for low power in the power control block */
	am_hal_pwrctrl_low_power_init();

	/* Disable the RTC. */
	am_hal_rtc_osc_disable();

#ifdef CONFIG_PM
	ambiq_power_init();
#endif
}
