/*
 * Copyright (c) 2025 Texas Instruments
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#ifdef CONFIG_HAS_MSPM0_SDK
#include <ti/driverlib/driverlib.h>
#endif /* CONFIG_HAS_MSPM0_SDK */

#include <soc.h>

/* The BL/BX instructions themselves will be worth 1-3 overhead cycles depending on optimisation */
void __noinline msp_delay_peripheral_startup(void)
{
#define NOP(i, ...)    "nop;"
#define NOP_N_TIMES(n) LISTIFY(n, NOP, ())

	__asm__ volatile(NOP_N_TIMES(CONFIG_MSPM0_PERIPH_STARTUP_DELAY));
}

#ifdef CONFIG_HAS_MSPM0_SDK
void soc_early_init_hook(void)
{
	/* Low Power Mode is configured to be SLEEP0 */
	DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

#if defined(CONFIG_SOC_MSPM0G1518) || defined(CONFIG_SOC_MSPM0G1519) ||                            \
	defined(CONFIG_SOC_MSPM0G3518) || defined(CONFIG_SOC_MSPM0G3519)
	/* FLASH_ERR_01: copy the factory trim table to SRAM before the clock tree is brought up */
	if (!DL_FactoryRegion_isTrimTableInSram()) {
		DL_FactoryRegion_initTrimTable();
	}
#endif
}
#endif /* CONFIG_HAS_MSPM0_SDK */
