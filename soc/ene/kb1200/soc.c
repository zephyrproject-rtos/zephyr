/*
 * Copyright (c) 2023 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <reg/pmu.h>
#include <reg/gcfg.h>

#define PMU_BASE DT_REG_ADDR(DT_NODELABEL(pmu))
#define GCFG_BASE DT_REG_ADDR(DT_NODELABEL(gcfg))

static void pmu_init(void)
{
	struct pmu_regs *pmu = ((struct pmu_regs *)PMU_BASE);

	/* Interrupt Event Wakeup from IDLE mode Enable */
	pmu->PMUIDLE |= PMU_IDLE_WU_ENABLE;
	/* GPTD wake up from STOP mode enable. */
	pmu->PMUSTOP |= PMU_STOP_WU_GPTD;
	/* SWD EDI32 wake up from STOP mode enable */
	pmu->PMUSTOP |= (PMU_STOP_WU_EDI32 | PMU_STOP_WU_SWD);
}
static void clock_init(void)
{
	struct gcfg_regs *gcfg = ((struct gcfg_regs *)GCFG_BASE);

	if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 96000000) {
		/* AHB/APB clock select 96MHz/48MHz */
		gcfg->CLKCFG = GCFG_CLKCFG_96M;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 48000000) {
		/* AHB/APB clock select 48MHz/24MHz */
		gcfg->CLKCFG = GCFG_CLKCFG_48M;
	} else {
		/* AHB/APB clock select 24MHz/12MHz */
		gcfg->CLKCFG = GCFG_CLKCFG_24M;
	}
}

static int kb1200_init(void)
{
	clock_init();
	pmu_init();
	return 0;
}

SYS_INIT(kb1200_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
