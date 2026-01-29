/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/nmi.h>
#include <zephyr/cache.h>
#include <bsp_api.h>

extern void NMI_Handler(void);

void soc_early_init_hook(void)
{
#ifdef CONFIG_RUNTIME_NMI
	z_arm_nmi_set_handler(NMI_Handler);
#endif /* CONFIG_RUNTIME_NMI */

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
