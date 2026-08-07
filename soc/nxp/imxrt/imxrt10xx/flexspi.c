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

/* Use FlexSPi internal divider for clock modification.
 * This function only updates the freq, but does not update DLL.
 * Use function memc_flexspi_update_clock to update both of them.
 */
uint32_t flexspi_clock_set_freq(uint32_t clock_name, uint32_t rate)
{
	uint8_t divider;
	uint32_t root_rate;
	FLEXSPI_Type *flexspi;

	switch (clock_name) {
	case IMX_CCM_FLEXSPI_CLK:
		/* Get clock root frequency */
		root_rate = CLOCK_GetClockRootFreq(kCLOCK_FlexspiClkRoot) *
					(CLOCK_GetDiv(kCLOCK_FlexspiDiv) + 1);
		flexspi = (FLEXSPI_Type *)DT_REG_ADDR(DT_NODELABEL(flexspi));
		break;
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexspi2))
	case IMX_CCM_FLEXSPI2_CLK:
		/* Get clock root frequency */
		root_rate = CLOCK_GetClockRootFreq(kCLOCK_Flexspi2ClkRoot) *
					(CLOCK_GetDiv(kCLOCK_Flexspi2Div) + 1);
		flexspi = (FLEXSPI_Type *)DT_REG_ADDR(DT_NODELABEL(flexspi2));
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
	/* Update the internal divider*/
	flexspi->MCR0 &= ~FLEXSPI_MCR0_SERCLKDIV_MASK;
	flexspi->MCR0 |= FLEXSPI_MCR0_SERCLKDIV(divider);
	FLEXSPI_Enable(flexspi, true);

	FLEXSPI_SoftwareReset(flexspi);

	return 0;
}
