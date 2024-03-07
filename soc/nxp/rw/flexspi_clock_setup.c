/*
 * Copyright 2022-2023 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include "flexspi_clock_setup.h"
#ifdef CONFIG_MEMC
#include <fsl_flexspi.h>
#include <fsl_clock.h>
#endif

/**
 * @brief Set flexspi clock
 */
void __ramfunc set_flexspi_clock(FLEXSPI_Type *base, uint32_t src, uint32_t divider)
{
	CLKCTL0->FLEXSPIFCLKSEL = CLKCTL0_FLEXSPIFCLKSEL_SEL(src);
	CLKCTL0->FLEXSPIFCLKDIV |=
		CLKCTL0_FLEXSPIFCLKDIV_RESET_MASK; /* Reset the divider counter */
	CLKCTL0->FLEXSPIFCLKDIV = CLKCTL0_FLEXSPIFCLKDIV_DIV(divider - 1);
	while ((CLKCTL0->FLEXSPIFCLKDIV) & CLKCTL0_FLEXSPIFCLKDIV_REQFLAG_MASK) {
	}
}
