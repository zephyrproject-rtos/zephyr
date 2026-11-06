/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon CAT1C SOC.
 */

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <cy_sysclk.h>
#include <cy_wdt.h>

/* Cortex-M7 TCM control registers */
#define IFX_ITCMCR_ADDR		0xE000EF90U
#define IFX_DTCMCR_ADDR		0xE000EF94U
#define IFX_TCMCR_INIT_VAL	0x2FU

/* CM7_0_CTL and CM7_1_CTL have the same layout */
static void cm7_core_init(volatile uint32_t *cm7_ctl)
{
	/* Allow write access to Vector Table Offset Register and ITCM/DTCM configuration register
	 * (CPUSS_CM7_X_CTL.PPB_LOCK[3] and CPUSS_CM7_X_CTL.PPB_LOCK[1:0])
	 */
	*cm7_ctl &= ~(0xB);

	__DSB();
	__ISB();

	/* Enable ITCM and DTCM */
	SCB->ITCMCR = SCB->ITCMCR | 0x7; /* Set ITCMCR.EN, .RMW and .RETEN fields */
	SCB->DTCMCR = SCB->DTCMCR | 0x7; /* Set DTCMCR.EN, .RMW and .RETEN fields */

	*cm7_ctl |= (0x1 << CPUSS_CM7_0_CTL_INIT_TCM_EN_Pos);
	*cm7_ctl |= (0x2 << CPUSS_CM7_0_CTL_INIT_TCM_EN_Pos);
	*cm7_ctl |= (0x1 << CPUSS_CM7_0_CTL_INIT_RMW_EN_Pos);
	*cm7_ctl |= (0x2 << CPUSS_CM7_0_CTL_INIT_RMW_EN_Pos);

	/* ITCMCR EN/RMW/RETEN enabled to access ITCM */
	__UNALIGNED_UINT32_WRITE((void const *)IFX_ITCMCR_ADDR, IFX_TCMCR_INIT_VAL);
	/* DTCMCR EN/RMW/RETEN enabled to access DTCM */
	__UNALIGNED_UINT32_WRITE((void const *)IFX_DTCMCR_ADDR, IFX_TCMCR_INIT_VAL);

	__DSB();
	__ISB();
}

void soc_prep_hook(void)
{
#ifdef CONFIG_ATOMIC_OPERATIONS_ARCH
	cyt4dn_atomic_init();
#endif

#ifdef CONFIG_SOC_CYT4DNJBZS_M7_1
	cm7_core_init(&CPUSS->CM7_1_CTL);
#elif CONFIG_SOC_CYT4DNJBZS_M7_0
	cm7_core_init(&CPUSS->CM7_0_CTL);
#else
#error "Not valid"
#endif

	(void)Cy_SysClk_PeriGroupSetSlaveCtl(3, CY_SYSCLK_PERI_GROUP_SL_CTL, 0xFFFFU);
	(void)Cy_SysClk_PeriGroupSetSlaveCtl(4, CY_SYSCLK_PERI_GROUP_SL_CTL, 0xFFFFU);
	(void)Cy_SysClk_PeriGroupSetSlaveCtl(5, CY_SYSCLK_PERI_GROUP_SL_CTL, 0xFFFFU);
	(void)Cy_SysClk_PeriGroupSetSlaveCtl(6, CY_SYSCLK_PERI_GROUP_SL_CTL, 0xFFFFU);
	(void)Cy_SysClk_PeriGroupSetSlaveCtl(8, CY_SYSCLK_PERI_GROUP_SL_CTL, 0xFFFFU);
	(void)Cy_SysClk_PeriGroupSetSlaveCtl(9, CY_SYSCLK_PERI_GROUP_SL_CTL, 0xFFFFU);
}

#ifdef CONFIG_SOC_PER_CORE_INIT_HOOK
void soc_per_core_init_hook(void)
{
	if (soc_cpu_index() != 0U) {
		cm7_core_init(&CPUSS->CM7_1_CTL);
	}

#ifdef CONFIG_SMP
	cyt4dn_smp_cpu_init();
#endif
}
#endif

void soc_early_init_hook(void)
{
	sys_cache_instr_enable();
	sys_cache_data_enable();
}
