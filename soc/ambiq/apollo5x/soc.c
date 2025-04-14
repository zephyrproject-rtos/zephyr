/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/cache.h>
#include "soc.h"

extern void ambiq_power_init(void);
void soc_early_init_hook(void)
{
	/* Enable Loop and branch info cache */
	SCB->CCR |= SCB_CCR_LOB_Msk;
	__DSB();
	__ISB();

	/* Internal timer15 for SPOT manager */
	IRQ_CONNECT(82, 0, hal_internal_timer_isr, 0, 0);

	/* Initialize for low power in the power control block */
	am_hal_pwrctrl_low_power_init();

	/* Enable SIMOBUCK for the apollo5 Family */
	am_hal_pwrctrl_control(AM_HAL_PWRCTRL_CONTROL_SIMOBUCK_INIT, NULL);

	/* Enable Icache*/
	sys_cache_instr_enable();

	/* Enable Dcache */
	sys_cache_data_enable();

#ifdef CONFIG_PM
	ambiq_power_init();
#endif

#ifdef CONFIG_LOG_BACKEND_SWO
	/* Select HFRC 48MHz for the TPIU clock source */
	MCUCTRL->DBGCTRL_b.DBGTPIUCLKSEL = MCUCTRL_DBGCTRL_DBGTPIUCLKSEL_HFRC_48MHz;
	MCUCTRL->DBGCTRL_b.DBGTPIUTRACEENABLE = MCUCTRL_DBGCTRL_DBGTPIUTRACEENABLE_EN;
#endif
}
