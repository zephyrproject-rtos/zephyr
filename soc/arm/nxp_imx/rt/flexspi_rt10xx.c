/*
 * Copyright (c) 2023, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fsl_clock.h>
#include <soc.h>

/* reimplementation of non-inline CLOCK_GetFreq(kCLOCK_Usb1PllPfd0Clk) */
static uint32_t clock_get_usb1_pll_pfd0_clk(void)
{
	uint32_t freq;

	if (!CLOCK_IsPllEnabled(CCM_ANALOG, kCLOCK_PllUsb1)) {
		return 0;
	}

	freq = CLOCK_GetPllBypassRefClk(CCM_ANALOG, kCLOCK_PllUsb1);

	if (CLOCK_IsPllBypassed(CCM_ANALOG, kCLOCK_PllUsb1)) {
		return freq;
	}

	freq *= ((CCM_ANALOG->PLL_USB1 & CCM_ANALOG_PLL_USB1_DIV_SELECT_MASK) != 0) ? 22 : 20;

	/* get current USB1 PLL PFD output frequency */
	freq /= (CCM_ANALOG->PFD_480 & CCM_ANALOG_PFD_480_PFD0_FRAC_MASK) >>
		CCM_ANALOG_PFD_480_PFD0_FRAC_SHIFT;
	freq *= 18;

	return freq;
}

void flexspi_clock_set_div(uint32_t value)
{
	CLOCK_DisableClock(kCLOCK_FlexSpi);

	CLOCK_SetDiv(kCLOCK_FlexspiDiv, value);

	CLOCK_EnableClock(kCLOCK_FlexSpi);
}

uint32_t flexspi_clock_get_freq(void)
{
	uint32_t divider;
	uint32_t frequency;

	divider = CLOCK_GetDiv(kCLOCK_FlexspiDiv);

	frequency = clock_get_usb1_pll_pfd0_clk() / (divider + 1);

	return frequency;
}
