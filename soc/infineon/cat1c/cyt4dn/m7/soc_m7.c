/*
 * Copyright (c) 2026 Linumiz
 * Copyright (c) 2026 Infineon Technologies AG
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon CAT1C SOC.
 */

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <cy_sysclk.h>
#include <cy_wdt.h>

/* Cortex-M7 TCM control registers */
#define IFX_ITCMCR_ADDR		0xE000EF90U
#define IFX_DTCMCR_ADDR		0xE000EF94U
#define IFX_TCMCR_INIT_VAL	0x2FU

void soc_prep_hook(void)
{
	/* disable global interrupt */
	__disable_irq();

	/* Allow write access to Vector Table Offset Register and ITCM/DTCM configuration register
	 * (CPUSS_CM7_X_CTL.PPB_LOCK[3] and CPUSS_CM7_X_CTL.PPB_LOCK[1:0])
	 */
#ifdef CONFIG_SOC_CYT4DN_CORE_M7_1
	CPUSS->CM7_1_CTL &= ~(0xB);
#elif CONFIG_SOC_CYT4DN_CORE_M7_0
	CPUSS->CM7_0_CTL &= ~(0xB);
#else
#error "Not valid"
#endif

	__DSB();
	__ISB();

	/* Enable ITCM and DTCM */
	SCB->ITCMCR = SCB->ITCMCR | 0x7; /* Set ITCMCR.EN, .RMW and .RETEN fields */
	SCB->DTCMCR = SCB->DTCMCR | 0x7; /* Set DTCMCR.EN, .RMW and .RETEN fields */

#ifdef CONFIG_SOC_CYT4DN_CORE_M7_0
	CPUSS_CM7_0_CTL |= (0x1 << CPUSS_CM7_0_CTL_INIT_TCM_EN_Pos);
	CPUSS_CM7_0_CTL |= (0x2 << CPUSS_CM7_0_CTL_INIT_TCM_EN_Pos);
	CPUSS_CM7_0_CTL |= (0x1 << CPUSS_CM7_0_CTL_INIT_RMW_EN_Pos);
	CPUSS_CM7_0_CTL |= (0x2 << CPUSS_CM7_0_CTL_INIT_RMW_EN_Pos);
#elif CONFIG_SOC_CYT4DN_CORE_M7_1
	CPUSS_CM7_1_CTL |= (0x1 << CPUSS_CM7_1_CTL_INIT_TCM_EN_Pos);
	CPUSS_CM7_1_CTL |= (0x2 << CPUSS_CM7_1_CTL_INIT_TCM_EN_Pos);
	CPUSS_CM7_1_CTL |= (0x1 << CPUSS_CM7_1_CTL_INIT_RMW_EN_Pos);
	CPUSS_CM7_1_CTL |= (0x2 << CPUSS_CM7_1_CTL_INIT_RMW_EN_Pos);
#else
#error "Not valid"
#endif

	/* ITCMCR EN/RMW/RETEN enabled to access ITCM */
	__UNALIGNED_UINT32_WRITE((void const *)IFX_ITCMCR_ADDR, IFX_TCMCR_INIT_VAL);
	/* DTCMCR EN/RMW/RETEN enabled to access DTCM */
	__UNALIGNED_UINT32_WRITE((void const *)IFX_DTCMCR_ADDR, IFX_TCMCR_INIT_VAL);

	__DSB();
	__ISB();

	(void)Cy_SysClk_PeriGroupSetSlaveCtl(3, CY_SYSCLK_PERI_GROUP_SL_CTL, 0xFFFFU);
	(void)Cy_SysClk_PeriGroupSetSlaveCtl(4, CY_SYSCLK_PERI_GROUP_SL_CTL, 0xFFFFU);
	(void)Cy_SysClk_PeriGroupSetSlaveCtl(5, CY_SYSCLK_PERI_GROUP_SL_CTL, 0xFFFFU);
	(void)Cy_SysClk_PeriGroupSetSlaveCtl(6, CY_SYSCLK_PERI_GROUP_SL_CTL, 0xFFFFU);
	(void)Cy_SysClk_PeriGroupSetSlaveCtl(8, CY_SYSCLK_PERI_GROUP_SL_CTL, 0xFFFFU);
	(void)Cy_SysClk_PeriGroupSetSlaveCtl(9, CY_SYSCLK_PERI_GROUP_SL_CTL, 0xFFFFU);

	Cy_WDT_Unlock();
	Cy_WDT_Disable();
	SystemCoreClockUpdate();
}

void soc_early_init_hook(void)
{
	sys_cache_instr_enable();
	sys_cache_data_enable();
}
