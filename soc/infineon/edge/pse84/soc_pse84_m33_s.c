/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon PSOC EDGE84 soc.
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include "soc.h"
#include <cy_sysint.h>
#include <system_edge.h>
#include <cy_pdl.h>

#include <pse84_boot.h>

#if defined(CONFIG_ARM_FIRMWARE_HAS_SECURE_CALLS)
#include <zephyr/arch/secure_domain.h>
#include <cmsis_core.h>

/*
 * PSE84 overrides only the vector-table write of the generic secure-domain
 * backend; the world-crossing hand-off itself is the arch SVC flow selected by
 * CONFIG_ARM_SECURE_DOMAIN_ENTRY_SVC (BXNS raises INVTRAN on this silicon).
 *
 * In addition to SCB_NS->VTOR, the Infineon CM33 wrapper keeps its own shadow
 * of the Non-Secure vector table base (MXCM33 CM33_NS_VECTOR_TABLE_BASE) which
 * must also be programmed.
 */
void z_arch_secure_domain_set_vector_table(uintptr_t base)
{
	SCB_NS->VTOR = (uint32_t)base;
	MXCM33->CM33_NS_VECTOR_TABLE_BASE = (uint32_t)base;
}
#endif /* CONFIG_ARM_FIRMWARE_HAS_SECURE_CALLS */

static void systeminit_enable_clocks(void)
{
	/* Void all return types to suppress compiler warnings about unused return values */

	/* For enabling SYS_MMIO_3, we need CLK_HF11 to be enabled first */
	(void)Cy_SysClk_ClkHfSetSource(11U, CY_SYSCLK_CLKHF_IN_CLKPATH0);
	(void)Cy_SysClk_ClkHfSetDivider(11U, CY_SYSCLK_CLKHF_NO_DIVIDE);
	(void)Cy_SysClk_ClkHfEnable(11U);

}

static void systeminit_enable_peri(void)
{
	/* Void all return types to suppress compiler warnings about unused return values */

	/* Release reset for all groups IP except group 0 */
	(void)Cy_SysClk_PeriGroupSetSlaveCtl((uint32_t)PERI_0_GROUP_1, CY_SYSCLK_PERI_GROUP_SL_CTL2,
					     0x0U);
	(void)Cy_SysClk_PeriGroupSetSlaveCtl((uint32_t)PERI_0_GROUP_2, CY_SYSCLK_PERI_GROUP_SL_CTL2,
					     0x0U);

	/* release reset of nnlite */
	(void)Cy_SysClk_PeriGroupSetSlaveCtl((uint32_t)PERI_0_GROUP_4, CY_SYSCLK_PERI_GROUP_SL_CTL2,
					     0x0U);

	(void)Cy_SysClk_PeriGroupSetSlaveCtl((uint32_t)PERI_0_GROUP_1, CY_SYSCLK_PERI_GROUP_SL_CTL,
					     0xFFFFFFFFU);
	(void)Cy_SysClk_PeriGroupSetSlaveCtl((uint32_t)PERI_0_GROUP_2, CY_SYSCLK_PERI_GROUP_SL_CTL,
					     0xFFFFFFFFU);

	/* enable nnlite */
	(void)Cy_SysClk_PeriGroupSetSlaveCtl((uint32_t)PERI_0_GROUP_4, CY_SYSCLK_PERI_GROUP_SL_CTL,
					     0x1U);

	(void)Cy_SysClk_PeriGroupSetSlaveCtl((uint32_t)PERI_0_GROUP_3, CY_SYSCLK_PERI_GROUP_SL_CTL,
					     0xFFFFFFFFU);

	/* Perform setup for CM33_NS and CM55 */
	(void)Cy_SysClk_PeriGroupSetSlaveCtl((uint32_t)PERI_1_GROUP_2, CY_SYSCLK_PERI_GROUP_SL_CTL,
					     0x3U);

	/* Release reset for all groups IP except group 0 */
	(void)Cy_SysClk_PeriGroupSetSlaveCtl((uint32_t)PERI_1_GROUP_1, CY_SYSCLK_PERI_GROUP_SL_CTL2,
					     0x0U);
	(void)Cy_SysClk_PeriGroupSetSlaveCtl((uint32_t)PERI_1_GROUP_2, CY_SYSCLK_PERI_GROUP_SL_CTL2,
					     0x0U);
	(void)Cy_SysClk_PeriGroupSetSlaveCtl((uint32_t)PERI_1_GROUP_0, CY_SYSCLK_PERI_GROUP_SL_CTL,
					     0xFFFFFFFFU);
	(void)Cy_SysClk_PeriGroupSetSlaveCtl((uint32_t)PERI_1_GROUP_1, CY_SYSCLK_PERI_GROUP_SL_CTL,
					     0xFFFFFFFFU);
	(void)Cy_SysClk_PeriGroupSetSlaveCtl((uint32_t)PERI_1_GROUP_2, CY_SYSCLK_PERI_GROUP_SL_CTL,
					     0xFFFFFFFFU);
}

void soc_early_init_hook(void)
{
	systeminit_enable_clocks();
	systeminit_enable_peri();

#ifdef CONFIG_FLASH_INFINEON_SMIF_HW_INIT
	/* Initialize power domain and peripheral group slave for SMIF init */
	Cy_System_EnablePD1();
	Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_SMIF0_PERI_NR, CY_MMIO_SMIF0_GROUP_NR,
				     CY_MMIO_SMIF0_SLAVE_NR, CY_MMIO_SMIF0_CLK_HF_NR);
#endif

	/* Initializes the system */
	SystemInit();
}

void soc_late_init_hook(void)
{
	/* SAU Init */
	cy_sau_init();

	/*
	 * Power up the SOCMEM domain so its MPC is accessible when the Secure
	 * image programs the memory protection controllers (the SOCMEM MPC node
	 * is DT-driven; see dts/arm/infineon/edge/pse84/pse84_s.dtsi).
	 */
	Cy_SysEnableSOCMEM(true);

#if defined(CONFIG_SOC_PSE84_M55_ENABLE)
	ifx_pse84_cm55_startup();
#endif
}
