/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Renesas RA8P1 family processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <cmsis_core.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/cache.h>
LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#include <bsp_api.h>

#define CCR_CACHE_ENABLE (SCB_CCR_IC_Msk | SCB_CCR_BP_Msk | SCB_CCR_LOB_Msk)

uint32_t SystemCoreClock BSP_SECTION_EARLY_INIT;

volatile uint32_t g_protect_pfswe_counter BSP_SECTION_EARLY_INIT;

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	SystemCoreClock = BSP_MOCO_HZ;
	g_protect_pfswe_counter = 0;

	extern volatile uint16_t g_protect_counters[];

	for (uint32_t i = 0; i < 5; i++) {
		g_protect_counters[i] = 0;
	}

	SystemCoreClock = BSP_MOCO_HZ;

#ifdef CONFIG_CPU_CORTEX_M85
#ifdef CONFIG_ICACHE
	SCB->CCR = (uint32_t)CCR_CACHE_ENABLE;
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
#endif
#if defined(CONFIG_DCACHE) && defined(CONFIG_CACHE_MANAGEMENT)
	/* Apply Arm Cortex-M85 errata workarounds for D-Cache
	 * Attributing all cacheable memory as write-through set FORCEWT bit in MSCR register.
	 * Set bit 16 in ACTLR to 1.
	 * See erratum 3175626 and 3190818 in the Cortex-M85 AT640 and Cortex-M85 with FPU AT641
	 * Software Developer Errata Notice (Date of issue: March 07, 2024, Document version: 13.0,
	 * Document ID: SDEN-2236668).
	 */
	MEMSYSCTL->MSCR |= MEMSYSCTL_MSCR_FORCEWT_Msk;
	barrier_dsync_fence_full();
	barrier_isync_fence_full();
	ICB->ACTLR |= (1U << 16U);
	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	sys_cache_data_enable();
#endif

#endif /*CONFIG_CPU_CORTEX_M85*/

#ifdef CONFIG_CPU_CORTEX_M33
#if FSP_PRIV_TZ_USE_SECURE_REGS
	/* Disable protection using PRCR register. */
	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_SAR);

	/* Initialize peripherals to secure mode for flat projects */
	R_PSCU->PSARB = 0;
	R_PSCU->PSARC = 0;
	R_PSCU->PSARD = 0;
	R_PSCU->PSARE = 0;

	R_CPSCU->ICUSARG = 0;
	R_CPSCU->ICUSARH = 0;
	R_CPSCU->ICUSARI = 0;

	/* Enable protection using PRCR register. */
	R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_SAR);
#endif
#endif /*CONFIG_CPU_CORTEX_M33*/
}

#ifdef CONFIG_SOC_LATE_INIT_HOOK
void soc_late_init_hook(void)
{
#ifdef CONFIG_SOC_RA_ENABLE_START_SECOND_CORE
	R_BSP_SecondaryCoreStart();
#endif /* CONFIG_SOC_RA_ENABLE_START_SECOND_CORE */
}
#endif /* CONFIG_SOC_LATE_INIT_HOOK */
