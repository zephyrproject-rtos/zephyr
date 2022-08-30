/*
 * Copyright (c) 2021 Katsuhiro Suzuki
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include "fu740_prci.h"

BUILD_ASSERT(MHZ(1000) == DT_PROP(DT_NODELABEL(coreclk), clock_frequency),
	"Unsupported CORECLK frequency");
BUILD_ASSERT(KHZ(125125) == DT_PROP(DT_NODELABEL(pclk), clock_frequency),
	"Unsupported PCLK frequency");

/*
 * Switch the clock source
 *   - core: to 1GHz PLL (CORE_PLL) from 26MHz oscillator (HFCLK)
 *   - peri: to 250MHz PLL (HFPCLKPLL) from HFCLK
 * on the HiFive Unmatched board.
 *
 * Note: Valid PLL VCO range is 2400MHz to 4800MHz
 */
static int fu740_clock_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	PRCI_REG(PRCI_COREPLLCFG) =
		PLL_R(0) |   /* input divider: Fin / (0 + 1) = 26MHz */
		PLL_F(76) |  /* VCO: 2 x (76 + 1) = 154 = 4004MHz */
		PLL_Q(2) |   /* output divider: VCO / 2^2 = 1001MHz */
		PLL_RANGE(PLL_RANGE_18MHZ) | /* 18MHz <= post divr(= 26MHz) < 30MHz */
		PLL_BYPASS(PLL_BYPASS_DISABLE) |
		PLL_FSE(PLL_FSE_INTERNAL);
	while ((PRCI_REG(PRCI_COREPLLCFG) & PLL_LOCK(1)) == 0)
		;

	/* Switch CORE_CLK to CORE_PLL from HFCLK */
	PRCI_REG(PRCI_COREPLLSEL) = COREPLLSEL_SEL(COREPLLSEL_COREPLL);
	PRCI_REG(PRCI_CORECLKSEL) = CLKSEL_SEL(CLKSEL_PLL);

	PRCI_REG(PRCI_HFPCLKPLLCFG) =
		PLL_R(0) |   /* input divider: Fin / (0 + 1) = 26MHz */
		PLL_F(76) |  /* VCO: 2 x (76 + 1) = 154 = 4004MHz */
		PLL_Q(4) |   /* output divider: VCO / 2^4 = 250.25MHz */
		PLL_RANGE(PLL_RANGE_18MHZ) | /* 18MHz <= post divr(= 26MHz) < 30MHz */
		PLL_BYPASS(PLL_BYPASS_DISABLE) |
		PLL_FSE(PLL_FSE_INTERNAL);
	while ((PRCI_REG(PRCI_HFPCLKPLLCFG) & PLL_LOCK(1)) == 0)
		;

	/* Switch PCLK to HFPCLKPLL/2 from HFCLK/2 */
	PRCI_REG(PRCI_HFPCLKPLLOUTDIV) = OUTDIV_PLLCKE(OUTDIV_PLLCKE_ENA);
	PRCI_REG(PRCI_HFPCLKPLLSEL) = CLKSEL_SEL(CLKSEL_PLL);

	return 0;
}

SYS_INIT(fu740_clock_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
