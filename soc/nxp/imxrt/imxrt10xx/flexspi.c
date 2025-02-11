/*
 * Copyright (c) 2023, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fsl_clock.h>
#include <fsl_flexspi.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/irq.h>
#include <zephyr/dt-bindings/clock/imx_ccm.h>

uint32_t flexspi_clock_set_freq(uint32_t clock_name, uint32_t rate)
{
	uint8_t divider;
	uint32_t root_rate;
	FLEXSPI_Type *flexspi;
	clock_div_t div_sel;
	clock_ip_name_t clk_name;

	switch (clock_name) {
	case IMX_CCM_FLEXSPI_CLK:
		/* Get clock root frequency */
		root_rate = CLOCK_GetClockRootFreq(kCLOCK_FlexspiClkRoot) *
					(CLOCK_GetDiv(kCLOCK_FlexspiDiv) + 1);
		flexspi = (FLEXSPI_Type *)DT_REG_ADDR(DT_NODELABEL(flexspi));
		div_sel = kCLOCK_FlexspiDiv;
		clk_name = kCLOCK_FlexSpi;
		break;
#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexspi2), okay)
	case IMX_CCM_FLEXSPI2_CLK:
		/* Get clock root frequency */
		root_rate = CLOCK_GetClockRootFreq(kCLOCK_Flexspi2ClkRoot) *
					(CLOCK_GetDiv(kCLOCK_Flexspi2Div) + 1);
		flexspi = (FLEXSPI_Type *)DT_REG_ADDR(DT_NODELABEL(flexspi2));
		div_sel = kCLOCK_Flexspi2Div;
		clk_name = kCLOCK_FlexSpi2;
		break;
#endif
	default:
		return -ENOTSUP;
	}
	/* Select a divider based on root frequency.
	 * if we can't get an exact divider, round down
	 */
	divider = ((root_rate + (rate - 1)) / rate) - 1;
	/* Cap divider to max value */
	divider = MIN(divider, kCLOCK_FlexspiDivBy8);

	while (FLEXSPI_GetBusIdleStatus(flexspi) == false) {
		/* Spin */
	}
	FLEXSPI_Enable(flexspi, false);

	CLOCK_DisableClock(clk_name);

	CLOCK_SetDiv(div_sel, divider);

	CLOCK_EnableClock(clk_name);

	FLEXSPI_Enable(flexspi, true);

	FLEXSPI_SoftwareReset(flexspi);

	return 0;
}
