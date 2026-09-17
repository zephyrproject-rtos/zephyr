/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/cache.h>
#include "soc.h"

#if defined(CONFIG_SOC_MCXE32B_CPU0)
#include <fsl_power.h>
#endif

void soc_early_init_hook(void)
{
#if defined(CONFIG_SOC_MCXE32B_CPU0)
	SystemInit();
	enable_sram_extra_latency(true);
	/* Enable I/DCache */
	sys_cache_instr_enable();
	sys_cache_data_enable();
#endif
}

#if defined(CONFIG_SOC_MCXE32B_CPU0)
void soc_late_init_hook(void)
{
	if (POWER_ExitFromStandbyMode()) {
		/* Re-latch IO controls before application output after standby reset. */
		DCM_GPR->DCMRWF1 |= (uint32_t)DCM_GPR_DCMRWF1_STANDBY_IO_CONFIG_MASK;
	}
}
#endif
