/*
 * Copyright (c) 2021 metraTec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <fsl_power.h>
#include <fsl_clock.h>

int soc_init(void)
{
	POWER_DisablePD(kPDRUNCFG_PD_FRO_EN);
	CLOCK_SetupFROClocking(12000000U);
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);

	CLOCK_SetFLASHAccessCyclesForFreq(12000000U);

	CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U, false);
	CLOCK_SetClkDiv(kCLOCK_DivSystickClk, 0U, true);
	CLOCK_SetClkDiv(kCLOCK_DivSystickClk, 1U, false);

	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);

	/* Attach 12 MHz clock to flexcomm0 */
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM0);

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm4), nxp_lpc_i2c, okay)
	/* attach 12 MHz clock for flexcomm4 */
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM4);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm5), nxp_lpc_spi, okay)
	/* attach 12MHz clock to flexcomm5 */
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM5);
#endif

	POWER_DisablePD(kPDRUNCFG_PD_ADC0);
	POWER_DisablePD(kPDRUNCFG_PD_VD7_ENA);
	POWER_DisablePD(kPDRUNCFG_PD_VREFP_SW);
	POWER_DisablePD(kPDRUNCFG_PD_TEMPS);

	return 0;
}

#ifdef CONFIG_PLATFORM_SPECIFIC_INIT

void z_arm_platform_init(void)
{
	SystemInit();
}

#endif /* CONFIG_PLATFORM_SPECIFIC_INIT */

SYS_INIT(soc_init, PRE_KERNEL_1, 0);
