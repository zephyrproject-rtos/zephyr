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

#include "soc.h"
#include <cy_sysint.h>
#include <system_edge.h>
#include "cy_pdl.h"

#include "pse84_boot.h"

#define CY_IPC_MAX_ENDPOINTS (8UL)

static void systeminit_enable_clocks(void)
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

	/* Initializes the system */
	SystemInit();
}

void soc_late_init_hook(void)
{
#if defined(CONFIG_SOC_PSE84_M55_ENABLE)
	ifx_pse84_cm55_startup();
#endif
}
