/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2017 Palmer Dabbelt <palmer@dabbelt.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include "fe310_prci.h"

#define CORECLK_HZ (DT_PROP(DT_NODELABEL(coreclk), clock_frequency))
BUILD_ASSERT(DT_PROP(DT_NODELABEL(tlclk), clock_div) == 1,
	"Unsupported TLCLK divider");

static int fe310_clock_init(void)
{

	/*
	 * HFXOSC (16 MHz) is used to produce coreclk (and therefore tlclk /
	 * peripheral clock). This code supports the following frequencies:
	 * - 16 MHz (bypass HFPLL).
	 * - 48 MHz - 320 MHz, in 8 MHz steps (use HFPLL).
	 */
	BUILD_ASSERT(MHZ(16) == CORECLK_HZ ||
		(MHZ(48) <= CORECLK_HZ && MHZ(320) >= CORECLK_HZ &&
		(CORECLK_HZ % MHZ(8)) == 0),
		"Unsupported CORECLK frequency");

	uint32_t prci;

	if (MHZ(16) == CORECLK_HZ) {
		/* Bypass HFPLL. */
		prci = PLL_REFSEL(1) | PLL_BYPASS(1);
	} else {
		/* refr = 8 MHz. */
		const int pll_r = 0x1;
		int pll_q;

		/* Select Q divisor to produce vco on [384 MHz, 768 MHz]. */
		if (MHZ(768) / 8 >= CORECLK_HZ) {
			pll_q = 0x3;
		} else if (MHZ(768) / 4 >= CORECLK_HZ) {
			pll_q = 0x2;
		} else {
			pll_q = 0x1;
		}
		/* Select F multiplier to produce vco target. */
		const int pll_f = ((CORECLK_HZ / MHZ(1)) >> (4 - pll_q)) - 1;

		prci = PLL_REFSEL(1) | PLL_R(pll_r) | PLL_F(pll_f) | PLL_Q(pll_q);
	}

	PRCI_REG(PRCI_PLLCFG) = prci;
	PRCI_REG(PRCI_PLLDIV) = (PLL_FINAL_DIV_BY_1(1) | PLL_FINAL_DIV(0));
	PRCI_REG(PRCI_PLLCFG) |= PLL_SEL(1);
	PRCI_REG(PRCI_HFROSCCFG) &= ~ROSC_EN(1);
	return 0;
}

SYS_INIT(fe310_clock_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
