/*
 * Copyright (c) 2021 ENE Technology.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#define CORTEX_M_SCR   0xE000ED10

static void pmu_init()
{
	PMU_T *pmu = ((PMU_T *) PMU_BASE);
	*((volatile uint32_t *)CORTEX_M_SCR)  &= (~(1L<<2));   // Disable SLEEPDEEP
	pmu->PMUIDLE |= 0x00000001;                            // Interrupt Event Wakeup from IDLE mode Enable
	pmu->PMUSTOP |= PMU_STOP_WU_GPTD;                      // GPTD wake up from STOP mode enable.
	pmu->PMUSTOP |= (PMU_STOP_WU_EDI32 | PMU_STOP_WU_SWD); // SWD EDI32 wake up from STOP mode enable
}

#define KB1200_CLKCFG 0x40000020

static void clock_init()
{
	if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 96000000)
		*((volatile uint32_t *)KB1200_CLKCFG) = 0x00000004;   // AHB/APB clock select 96MHz/48MHz
	else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 48000000)
		*((volatile uint32_t *)KB1200_CLKCFG) = 0x00000014;   // AHB/APB clock select 48MHz/24MHz
	else
		*((volatile uint32_t *)KB1200_CLKCFG) = 0x00000024;   // AHB/APB clock select 24MHz/12MHz
}

static int ene_kb1200_init(void)
{
	clock_init();
	pmu_init();
	return 0;
}

SYS_INIT(ene_kb1200_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
