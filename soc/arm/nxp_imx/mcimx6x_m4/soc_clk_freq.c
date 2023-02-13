/*
 * Copyright (c) 2021, Antonio Tessarolo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ccm_imx6sx.h>
#include <ccm_analog_imx6sx.h>
#include "soc_clk_freq.h"

#ifdef CONFIG_PWM_IMX

uint32_t get_pwm_clock_freq(PWM_Type *base)
{
	uint32_t root;
	uint32_t hz;
	uint32_t divPerclkPodf, divIpgPodf, divAhbPodf, divPeriphClk2Podf;

	/* Different instance has the same clock root, it's different from i.mx7d. */
	/* Get the clock root according to the mux node of clock tree. */
	if (CCM_GetRootMux(CCM, ccmRootPerclkClkSel) ==
		ccmRootmuxPerclkClkOsc24m) {
		root = ccmRootmuxPerclkClkOsc24m;
		hz = 24000000;
		divPerclkPodf = CCM_GetRootDivider(CCM, ccmRootPerclkPodf);
		divIpgPodf = 0;
		divAhbPodf = 0;
		divPeriphClk2Podf = 0;
	} else if (CCM_GetRootMux(CCM, ccmRootPeriphClkSel) ==
			   ccmRootmuxPeriphClkPrePeriphClkSel) {
		root = CCM_GetRootMux(CCM, ccmRootPrePeriphClkSel);
		/* Here do not show all the clock root source,
		 * if user use other clock root source, such as PLL2_PFD2, please
		 * add it as follows according to the clock tree of CCM in reference manual.
		 */
		switch (root) {
		case ccmRootmuxPrePeriphClkPll2:
			hz = CCM_ANALOG_GetPllFreq(CCM_ANALOG, ccmAnalogPllSysControl);
			divPerclkPodf = CCM_GetRootDivider(CCM, ccmRootPerclkPodf);
			divIpgPodf = CCM_GetRootDivider(CCM, ccmRootIpgPodf);
			divAhbPodf = CCM_GetRootDivider(CCM, ccmRootAhbPodf);
			divPeriphClk2Podf = 0;
			break;
		default:
			return 0;
		}
	} else if (CCM_GetRootMux(CCM, ccmRootPeriphClk2Sel) ==
			   ccmRootmuxPeriphClk2OSC24m) {
		root = ccmRootmuxPeriphClk2OSC24m;
		hz = 24000000;
		divPerclkPodf = CCM_GetRootDivider(CCM, ccmRootPerclkPodf);
		divIpgPodf = CCM_GetRootDivider(CCM, ccmRootIpgPodf);
		divAhbPodf = CCM_GetRootDivider(CCM, ccmRootAhbPodf);
		divPeriphClk2Podf = CCM_GetRootDivider(CCM, ccmRootPeriphClk2Podf);
	} else {
		root = CCM_GetRootMux(CCM, ccmRootPll3SwClkSel);
		/* Here do not show all the clock root source,
		 * if user use other clock root source, such as PLL3_BYP, please
		 * add it as follows according to the clock tree of CCM in reference manual.
		 */
		switch (root) {
		case ccmRootmuxPll3SwClkPll3:
			hz = CCM_ANALOG_GetPllFreq(CCM_ANALOG, ccmAnalogPllUsb1Control);
			divPerclkPodf = CCM_GetRootDivider(CCM, ccmRootPerclkPodf);
			divIpgPodf = CCM_GetRootDivider(CCM, ccmRootIpgPodf);
			divAhbPodf = CCM_GetRootDivider(CCM, ccmRootAhbPodf);
			divPeriphClk2Podf =
					CCM_GetRootDivider(CCM, ccmRootPeriphClk2Podf);
			break;
		default:
			return 0;
		}
	}

	return hz / (divPerclkPodf + 1) / (divIpgPodf + 1) /
		   (divAhbPodf + 1) / (divPeriphClk2Podf + 1);
}

#endif /* CONFIG_PWM_IMX */
