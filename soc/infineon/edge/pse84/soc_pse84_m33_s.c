/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon PSOC EDGE84 soc.
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <cy_sysint.h>
#include <system_edge.h>
#include "cy_pdl.h"
#include "soc.h"

#include "pse84_boot.h"

#define CY_IPC_MAX_ENDPOINTS (8UL)

cy_en_sysint_status_t Cy_SysInt_Init(const cy_stc_sysint_t *config, cy_israddress userIsr)
{
	CY_ASSERT_L3(CY_SYSINT_IS_PRIORITY_VALID(config->intrPriority));
	cy_en_sysint_status_t status = CY_SYSINT_SUCCESS;

	/* The interrupt vector will be relocated only if the vector table was
	 * moved to SRAM (CONFIG_DYNAMIC_INTERRUPTS and CONFIG_GEN_ISR_TABLES
	 * must be enabled). Otherwise it is ignored.
	 */

#if (CY_CPU_CORTEX_M0P)
#error Cy_SysInt_Init does not support CM0p core.
#endif /* (CY_CPU_CORTEX_M0P) */

#if defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_GEN_ISR_TABLES)
	if (config != NULL) {
		uint32_t priority;

		/* NOTE:
		 * PendSV IRQ (which is used in Cortex-M variants to implement thread
		 * context-switching) is assigned the lowest IRQ priority level.
		 * If priority is same as PendSV, we will catch assertion in
		 * z_arm_irq_priority_set function. To avoid this, change priority
		 * to IRQ_PRIO_LOWEST, if it > IRQ_PRIO_LOWEST. Macro IRQ_PRIO_LOWEST
		 * takes in to account PendSV specific.
		 */
		priority = (config->intrPriority > IRQ_PRIO_LOWEST) ? IRQ_PRIO_LOWEST
								    : config->intrPriority;

		/* Configure a dynamic interrupt */
		(void)irq_connect_dynamic(config->intrSrc, priority, (void *)userIsr, NULL, 0);
	} else {
		status = CY_SYSINT_BAD_PARAM;
	}
#endif /* defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_GEN_ISR_TABLES) */

	return (status);
}

static void SystemInit_Enable_Clocks(void)
{
	/* Void all return types to suppress compiler warnings about unused return values */

	/* For enabling SYS_MMIO_3, we need CLK_HF11 to be enabled first */
	(void)Cy_SysClk_ClkHfSetSource(11U, CY_SYSCLK_CLKHF_IN_CLKPATH0);
	(void)Cy_SysClk_ClkHfSetDivider(11U, CY_SYSCLK_CLKHF_NO_DIVIDE);
	(void)Cy_SysClk_ClkHfEnable(11U);

	/* enable HF3 and HF4 for SMIF */
	(void)Cy_SysClk_ClkHfSetSource(3U, CY_SYSCLK_CLKHF_IN_CLKPATH0);
	(void)Cy_SysClk_ClkHfSetDivider(3U, CY_SYSCLK_CLKHF_NO_DIVIDE);
	(void)Cy_SysClk_ClkHfEnable(3U);

	(void)Cy_SysClk_ClkHfSetSource(4U, CY_SYSCLK_CLKHF_IN_CLKPATH0);
	(void)Cy_SysClk_ClkHfSetDivider(4U, CY_SYSCLK_CLKHF_NO_DIVIDE);
	(void)Cy_SysClk_ClkHfEnable(4U);
}

static void SystemInit_Enable_Peri(void)
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
	SystemInit_Enable_Clocks();
	SystemInit_Enable_Peri();

	/* Initializes the system */
	SystemInit();
}

void soc_late_init_hook(void)
{
#if defined(CONFIG_SOC_PSE84_M33_NS_ENABLE)
	ifx_pse84_cm33_ns_startup();
#endif
}
