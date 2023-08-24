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
#include <zephyr/dt-bindings/clock/imx_ccm_rev2.h>

uint32_t flexspi_clock_set_freq(uint32_t clock_name, uint32_t rate)
{
	clock_name_t root;
	uint32_t root_rate;
	FLEXSPI_Type *flexspi;
	clock_root_t flexspi_clk;
	clock_ip_name_t clk_gate;
	uint32_t divider;

	switch (clock_name) {
	case IMX_CCM_FLEXSPI_CLK:
		flexspi_clk = kCLOCK_Root_Flexspi1;
		flexspi = (FLEXSPI_Type *)DT_REG_ADDR(DT_NODELABEL(flexspi));
		clk_gate = kCLOCK_Flexspi1;
		break;
	case IMX_CCM_FLEXSPI2_CLK:
		flexspi_clk = kCLOCK_Root_Flexspi2;
		flexspi = (FLEXSPI_Type *)DT_REG_ADDR(DT_NODELABEL(flexspi2));
		clk_gate = kCLOCK_Flexspi2;
		break;
	default:
		return -ENOTSUP;
	}
	root = CLOCK_GetRootClockSource(flexspi_clk,
			CLOCK_GetRootClockMux(flexspi_clk));
	/* Get clock root frequency */
	root_rate = CLOCK_GetFreq(root);
	/* Select a divider based on root frequency */
	divider = MIN((root_rate / rate), CCM_CLOCK_ROOT_CONTROL_DIV_MASK);

	while (FLEXSPI_GetBusIdleStatus(flexspi) == false) {
		/* Spin */
	}
	FLEXSPI_Enable(flexspi, false);

	CLOCK_DisableClock(clk_gate);

	CLOCK_SetRootClockDiv(flexspi_clk, divider);

	CLOCK_EnableClock(clk_gate);

	FLEXSPI_Enable(flexspi, true);

	FLEXSPI_SoftwareReset(flexspi);

	return 0;
}
