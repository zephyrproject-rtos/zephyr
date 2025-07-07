/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <soc.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

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

#ifdef CONFIG_LOG_BACKEND_SWO
	/* Select HFRC 48MHz for the TPIU clock source */
	MCUCTRL->DBGCTRL_b.CM4CLKSEL = MCUCTRL_DBGCTRL_CM4CLKSEL_HFRC48;
	MCUCTRL->DBGCTRL_b.CM4TPIUENABLE = MCUCTRL_DBGCTRL_CM4TPIUENABLE_EN;
#endif
}
