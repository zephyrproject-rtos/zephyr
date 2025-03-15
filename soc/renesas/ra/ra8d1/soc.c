/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Renesas RA8D1 family processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <cmsis_core.h>
#include <zephyr/arch/arm/nmi.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/cache.h>
LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#include <bsp_api.h>

#define CCR_CACHE_ENABLE (SCB_CCR_IC_Msk | SCB_CCR_BP_Msk | SCB_CCR_LOB_Msk)

uint32_t SystemCoreClock BSP_SECTION_EARLY_INIT;

volatile uint32_t g_protect_pfswe_counter BSP_SECTION_EARLY_INIT;

#ifdef CONFIG_RUNTIME_NMI
extern bsp_grp_irq_cb_t g_bsp_group_irq_sources[];
extern void NMI_Handler(void);
#endif /* CONFIG_RUNTIME_NMI */

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	SystemCoreClock = BSP_MOCO_HZ;
	g_protect_pfswe_counter = 0;

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

#ifdef CONFIG_RUNTIME_NMI
	for (uint32_t i = 0; i < 16; i++) {
		g_bsp_group_irq_sources[i] = 0;
	}

	z_arm_nmi_set_handler(NMI_Handler);
#endif /* CONFIG_RUNTIME_NMI */
}
