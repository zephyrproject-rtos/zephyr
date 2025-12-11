/*
 * Copyright 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fsl_common.h>

uint32_t flexspi_clock_set_freq(uint32_t clock_name, uint32_t rate)
{
	/* PLL0 is set to 150 MHz */
	uint32_t pll_rate = 150000000;
	uint8_t divider;

	/* Disable the FLEXSPI clock */
	SYSCON->FLEXSPICLKSEL = 0;

	divider = ((pll_rate + (rate - 1)) / rate) - 1;
	/* Max divider value is 8 */
	divider = MIN(divider, 8);
	SYSCON->FLEXSPICLKDIV = divider;

	/* Switch FLEXSPI to PLL0 */
	SYSCON->FLEXSPICLKSEL = 1;

	return 0;
}

void flexspi_clock_safe_config(void)
{
	/* Switch FLEXSPI to FRO_HF */
	SYSCON->FLEXSPICLKSEL = 3;

}
