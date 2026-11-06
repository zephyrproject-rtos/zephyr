/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/nmi.h>
#include <zephyr/cache.h>
#include <zephyr/sys/barrier.h>
#include <bsp_api.h>

extern void NMI_Handler(void);

void renesas_ra_reset_hook(void)
{
	/*
	 * SystemInit() runs from z_arm_reset, before z_arm_floating_point_init()
	 * and the compiler may emit FP/MVE instructions in it, which fault with NOCP.
	 * Grant privileged access to CP10/CP11 first; prep_c reprograms CPACR afterwards.
	 */
	if (IS_ENABLED(CONFIG_FPU)) {
		SCB->CPACR |= CPACR_CP10_PRIV_ACCESS | CPACR_CP11_PRIV_ACCESS;
		barrier_dsync_fence_full();
		barrier_isync_fence_full();
	}

	SystemInit();
}

void soc_early_init_hook(void)
{
#ifdef CONFIG_RUNTIME_NMI
	z_arm_nmi_set_handler(NMI_Handler);
#endif /* CONFIG_RUNTIME_NMI */

#if defined(CONFIG_DCACHE)
	sys_cache_data_enable();
#endif /* CONFIG_DCACHE */

#if defined(CONFIG_ICACHE)
	/* Invalidate I-Cache after initializing the .ram_from_flash section. */
	sys_cache_instr_invd_all();
#endif /* CONFIG_ICACHE */
}

void soc_late_init_hook(void)
{
#ifdef CONFIG_SOC_RA_ENABLE_START_SECOND_CORE
	R_BSP_SecondaryCoreStart();
#endif /* CONFIG_SOC_RA_ENABLE_START_SECOND_CORE */
}
