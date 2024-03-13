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

#ifdef CONFIG_MEMC
/**
 * @brief Set flexspi clock to given frequency
 *
 * This function differs from the above function in that it will not configure
 * the FlexSPI with a new MUX source, only change the divider. This function
 * is used by the clock control framework to set the clock frequency of
 * the FlexSPI
 */
int __ramfunc flexspi_clock_set_freq(uint32_t clock_name, uint32_t rate)
{
	uint32_t root_rate;
	uint32_t divider;

	(void)clock_name; /* Not used by this function */

	/* Get the root clock rate: FlexSPI clock * divisor */
	root_rate = ((CLKCTL0->FLEXSPIFCLKDIV & CLKCTL0_FLEXSPIFCLKDIV_DIV_MASK) + 1) *
			CLOCK_GetFlexspiClkFreq();

	/* Select a divider based on root frequency.
	 * if we can't get an exact divider, round down
	 */
	divider = ((root_rate + (rate - 1)) / rate) - 1;
	/* Cap divider to max value */
	divider = MIN(divider, CLKCTL0_FLEXSPIFCLKDIV_DIV_MASK);

	while (FLEXSPI_GetBusIdleStatus(FLEXSPI) == false) {
		/* Spin */
	}
	FLEXSPI_Enable(FLEXSPI, false);

	set_flexspi_clock(FLEXSPI, (CLKCTL0->FLEXSPIFCLKSEL &
			CLKCTL0_FLEXSPIFCLKDIV_DIV_MASK), (divider + 1));


	FLEXSPI_Enable(FLEXSPI, true);

	FLEXSPI_SoftwareReset(FLEXSPI);

	return 0;
}
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
