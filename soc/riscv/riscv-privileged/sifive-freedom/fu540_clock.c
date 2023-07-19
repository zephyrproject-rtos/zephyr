/*
 * Copyright (c) 2021 Katsuhiro Suzuki
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include "fu540_prci.h"

BUILD_ASSERT(MHZ(1000) == DT_PROP(DT_NODELABEL(coreclk), clock_frequency),
	"Unsupported CORECLK frequency");
BUILD_ASSERT(DT_PROP(DT_NODELABEL(tlclk), clock_div) == 2,
	"Unsupported TLCLK divider");

/*
 * Switch the clock source to 1GHz PLL from 33.333MHz oscillator on the HiFive
 * Unleashed board.
 */
static int fu540_clock_init(void)
{

	PRCI_REG(PRCI_COREPLLCFG0) =
		PLL_R(0) |   /* input divider: Fin / (0 + 1) = 33.33MHz */
		PLL_F(59) |  /* VCO: 2 x (59 + 1) = 120 = 3999.6MHz */
		PLL_Q(2) |   /* output divider: VCO / 2^2 = 999.9MHz */
		PLL_RANGE(PLL_RANGE_33MHZ) |
		PLL_BYPASS(PLL_BYPASS_DISABLE) |
		PLL_FSE(PLL_FSE_INTERNAL);
	while ((PRCI_REG(PRCI_COREPLLCFG0) & PLL_LOCK(1)) == 0)
		;

	/* Switch clock to COREPLL */
	PRCI_REG(PRCI_CORECLKSEL) = CORECLKSEL_CORECLKSEL(CORECLKSEL_CORE_PLL);

	return 0;
}

SYS_INIT(fu540_clock_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
