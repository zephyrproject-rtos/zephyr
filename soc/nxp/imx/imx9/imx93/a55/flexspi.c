/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fsl_clock.h>
#include <fsl_flexspi.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/irq.h>
#include <zephyr/dt-bindings/clock/imx_ccm_rev2.h>

#define MCR0_SERCLKDIV_MASK  0x700
#define MCR0_SERCLKDIV_SHIFT 8

uint32_t flexspi_clock_set_freq(uint32_t clock_name, uint32_t rate)
{
	uint32_t root_rate;
	FLEXSPI_Type *flexspi;
	clock_root_t flexspi_clk;
	clock_ip_name_t clk_gate;
	uint32_t divider;

	switch (clock_name) {
	case IMX_CCM_FLEXSPI_CLK:
		flexspi_clk = kCLOCK_Root_Flexspi1;
		flexspi = (FLEXSPI_Type *)DT_REG_ADDR(DT_NODELABEL(flexspi1));
		clk_gate = kCLOCK_Flexspi1;
		break;
	default:
		return -ENOTSUP;
	}

	/* Get clock root frequency */
	root_rate = CLOCK_GetIpFreq(flexspi_clk);

	/* Select a divider based on root clock frequency. We round the
	 * divider up, so that the resulting clock frequency is lower than
	 * requested when we can't output the exact requested frequency
	 */
	divider = ((root_rate + (rate - 1)) / rate);
	/* Cap divider to max value */
	divider = MIN(divider, 7U);

	while (FLEXSPI_GetBusIdleStatus(flexspi) == false) {
		/* Spin */
	}

	FLEXSPI_Enable(flexspi, false);

	flexspi->MCR0 &= ~MCR0_SERCLKDIV_MASK;
	flexspi->MCR0 |= divider << MCR0_SERCLKDIV_SHIFT;

	FLEXSPI_Enable(flexspi, true);

	FLEXSPI_SoftwareReset(flexspi);

	return 0;
}
