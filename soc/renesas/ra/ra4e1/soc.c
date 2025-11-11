/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Renesas RA4E1 family processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <cmsis_core.h>
#include <zephyr/arch/arm/nmi.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#include "bsp_cfg.h"
#include <bsp_api.h>

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
	extern volatile uint16_t g_protect_counters[];

	for (uint32_t i = 0; i < 5; i++) {
		g_protect_counters[i] = 0;
	}

#if FSP_PRIV_TZ_USE_SECURE_REGS
	/* Disable protection using PRCR register. */
	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_SAR);

	/* Initialize peripherals to secure mode for flat projects */
	R_PSCU->PSARB = 0;
	R_PSCU->PSARC = 0;
	R_PSCU->PSARD = 0;
	R_PSCU->PSARE = 0;

	R_CPSCU->ICUSARC = 0;
	R_CPSCU->ICUSARG = 0;
	R_CPSCU->ICUSARH = 0;
	R_CPSCU->ICUSARI = 0;

	/* Enable protection using PRCR register. */
	R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_SAR);
#endif

	SystemCoreClock = BSP_MOCO_HZ;
	g_protect_pfswe_counter = 0;

#ifdef CONFIG_RUNTIME_NMI
	for (uint32_t i = 0; i < 16; i++) {
		g_bsp_group_irq_sources[i] = 0;
	}

	z_arm_nmi_set_handler(NMI_Handler);
#endif /* CONFIG_RUNTIME_NMI */
}
