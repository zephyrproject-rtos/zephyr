/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 Linumiz GmbH
 * Author: Sri Surya  <srisurya@linumiz.com>
 */

#include <zephyr/init.h>
#include <soc.h>

void soc_early_init_hook(void)
{
	/* Set the clock frequency.*/
	am_hal_clkgen_sysclk_select(AM_HAL_CLKGEN_SYSCLK_MAX);

	/* Enable Flash cache.*/
	am_hal_cachectrl_enable(&am_hal_cachectrl_defaults);

	/* Initialize for low power in the power control block */
	am_hal_pwrctrl_low_power_init();

	/* Disable the RTC. */
	am_hal_rtc_osc_disable();
}
