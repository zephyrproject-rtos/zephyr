/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2017 Palmer Dabbelt <palmer@dabbelt.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <board.h>
#include "prci.h"

/* Selects the 16MHz oscilator on the HiFive1 board, which provides a clock
 * that's accurate enough to actually drive serial ports off of.
 */
static int hifive1_clock_init(struct device *dev)
{
	ARG_UNUSED(dev);

	/* Select external 16 MHz oscillator */
	PRCI_REG(PRCI_PLLCFG) = PLL_REFSEL(1);

	/*
	 * Configure PLL for 256 MHz
	 *
	 * R = 2, F = 64, Q = 2
	 * 16 * 1/2 * 64 * 1/2 = 256
	 *
	 * R and Q are encoded as one less than their desired value
	 * F is encoded as N, where F = 2*(N+1)
	 *
	 * Bypass the final divider
	 */
	PRCI_REG(PRCI_PLLCFG) |= (PLL_R(1) | PLL_F(31) | PLL_Q(1));
	PRCI_REG(PRCI_PLLDIV) = (PLL_FINAL_DIV_BY_1(1) | PLL_FINAL_DIV(0));

	/* Wait for PLL to lock */
	while((PRCI_REG(PRCI_PLLCFG)) & PLL_LOCK(1) == 0) ;

	/* Switch to PLL and disable the internal oscillator */
	PRCI_REG(PRCI_PLLCFG) |= PLL_SEL(1);
	PRCI_REG(PRCI_HFROSCCFG) &= ~ROSC_EN(1);

	return 0;
}

SYS_INIT(hifive1_clock_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
