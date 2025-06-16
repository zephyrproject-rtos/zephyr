/*
 * Copyright (c) 2024 Ian Morris
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Renesas RA4M1 family processor
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

#define HOCO_FREQ DT_PROP(DT_PATH(clocks, clock_hoco), clock_frequency)

#if HOCO_FREQ == MHZ(24)
#define OFS1_HOCO_FREQ 0
#elif HOCO_FREQ == MHZ(32)
#define OFS1_HOCO_FREQ 2
#elif HOCO_FREQ == MHZ(48)
#define OFS1_HOCO_FREQ 4
#elif HOCO_FREQ == MHZ(64)
#define OFS1_HOCO_FREQ 5
#else
#error "Unsupported HOCO frequency"
#endif

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
	uint32_t key;

	key = irq_lock();

	SystemCoreClock = BSP_MOCO_HZ;
	g_protect_pfswe_counter = 0;

#ifdef CONFIG_RUNTIME_NMI
	for (uint32_t i = 0; i < 16; i++) {
		g_bsp_group_irq_sources[i] = 0;
	}

	z_arm_nmi_set_handler(NMI_Handler);
#endif /* CONFIG_RUNTIME_NMI */

	irq_unlock(key);
}
